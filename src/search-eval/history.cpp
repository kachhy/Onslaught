#include "history.h"
#include "spsa.h"
#include "eval.h"
#include <algorithm>
#include <cstring>

int score_history[2][64][64]; // [stm][from][to] (butterfly history)
int cont_hist[2][6][64][6][64]; // [stm][prevPiece][prevTo][piece][to] (continuation history)
int capture_hist[12][64][6]; // [moving piece][to][captured type] (capture history)

thread_local int kp_corrhist[CORR_HIST_SIZE][2]; // [pawn hash][stm]

static inline int corrHistIndex(const Board& board) {
    return board.pawnHash() & (CORR_HIST_SIZE - 1);
}

void resetCorrHist() {
    memset(kp_corrhist, 0, sizeof(kp_corrhist));
}

int correctEval(const Board& board, int raw_eval) {
    const int corrected = raw_eval + kp_corrhist[corrHistIndex(board)][board.getSTM()] / CORR_HIST_SCALE;
    return std::clamp(corrected, -TB_WIN_SCORE + MAX_PLY + 1, TB_WIN_SCORE - MAX_PLY - 1); // Never look like a TB/mate score
}

void updateCorrHist(const Board& board, int raw_eval, int best_score, int depth) {
    int& entry = kp_corrhist[corrHistIndex(board)][board.getSTM()];
    
    // Error is measured against the raw eval
    const int error = (best_score - raw_eval) * CORR_HIST_SCALE;
    const int weight = 2 * std::min(depth + 1, 16);
    const int weighted = (entry * (CORR_HIST_SCALE - weight) + error * weight) / CORR_HIST_SCALE;

    // Limit how far a single update can move the entry, then keep it in range
    entry = std::clamp(std::clamp(weighted, entry - CORR_MAX_BONUS, entry + CORR_MAX_BONUS), -CORR_HIST_MAX, CORR_HIST_MAX);
}

void resetHistory() {
    memset(score_history, 0, sizeof(score_history));
    memset(cont_hist, 0, sizeof(cont_hist));
    memset(capture_hist, 0, sizeof(capture_hist));
}

void updateScoreHistory(int depth, Side stm, Move move, const MoveList& quiets_tried) {
    const int bonus = std::min(depth * depth, MAX_HISTORY);
    auto& score_hist = score_history[stm][From(move)][To(move)];
    score_hist += bonus - (score_hist * std::abs(bonus) / MAX_HISTORY);

    // malus: penalize all quiet moves searched before this cutoff
    for (const Move quiet_move : quiets_tried) {
        if (quiet_move == move) { // We just gave this move a bonus
            continue;
        }

        auto& score_hist = score_history[stm][From(quiet_move)][To(quiet_move)];
        score_hist += -bonus - (score_hist * std::abs(bonus) / MAX_HISTORY);
    }
}

static inline DefaultPiece capturedType(const Board& board, Move move) {
    // En passant: target square is empty, victim is always a pawn
    return IsEP(move) ? PAWN : makeDefaultPiece(board.pieceAt(To(move)));
}

void applyCaptureMalus(const Board& board, int depth, const MoveList& captures_tried) {
    const int malus = -std::min(depth * depth, MAX_HISTORY);
    // malus: penalize all loud moves searched before this cutoff (didn't cause a cutoff)
    for (const Move capture : captures_tried) {
        auto& h = capture_hist[MovePiece(capture)][To(capture)][capturedType(board, capture)];
        h += malus - (h * std::abs(malus) / MAX_HISTORY);
    }
}

void updateCaptureHistory(const Board& board, int depth, Move move, const MoveList& captures_tried) {
    const int bonus = std::min(depth * depth, MAX_HISTORY);
    auto& cap_hist = capture_hist[MovePiece(move)][To(move)][capturedType(board, move)];
    cap_hist += bonus - (cap_hist * std::abs(bonus) / MAX_HISTORY);

    // malus: penalize all loud moves searched before this cutoff (didn't cause a cutoff)
    for (const Move capture : captures_tried) {
        if (capture == move) { // We just gave this move a bonus
            continue;
        }

        auto& h = capture_hist[MovePiece(capture)][To(capture)][capturedType(board, capture)];
        h += -bonus - (h * std::abs(bonus) / MAX_HISTORY);
    }
}

int getScoreHistory(Side stm, Move move) {
    return score_history[stm][From(move)][To(move)];
}

int getCaptureHistory(Move move, DefaultPiece capturing) {
    return capture_hist[MovePiece(move)][To(move)][capturing] / CAPTURE_HISTORY_DIVISOR;
}

void updateContHistory(int depth, Side stm, Move move, SearchStack* ss, const MoveList& quiets_tried) {
    const int bonus = std::min(depth * depth, MAX_HISTORY);
    const DefaultPiece pc = makeDefaultPiece(MovePiece(move));
    const Square to = To(move);

    for (const int offset : { 1 }) {
        const Move prev_move = (ss - offset)->move;
        const DefaultPiece prev_pc = makeDefaultPiece(MovePiece(prev_move));
        const Square prev_to = To(prev_move);

        auto& h = cont_hist[stm][prev_pc][prev_to][pc][to];
        h += bonus - (h * std::abs(bonus) / MAX_HISTORY);

        // malus: penalize all quiet moves searched before this cutoff
        for (const Move quiet_move : quiets_tried) {
            if (quiet_move == move) { // We just gave this move a bonus
                continue;
            }

            auto& hm = cont_hist[stm][prev_pc][prev_to][makeDefaultPiece(MovePiece(quiet_move))][To(quiet_move)];
            hm += -bonus - (hm * std::abs(bonus) / MAX_HISTORY);
        }
    }
}

int getContHist(SearchStack* ss, Side stm, Move move) {
    const DefaultPiece pc = makeDefaultPiece(MovePiece(move));
    const Square to = To(move);
    return cont_hist[stm][makeDefaultPiece(MovePiece((ss - 1)->move))][To((ss - 1)->move)][pc][to];
}