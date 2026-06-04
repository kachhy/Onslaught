#include "history.h"
#include <cstring>

int score_history[2][64][64]; // [stm][from][to] (butterfly history)
int cont_hist[2][6][64][6][64]; // [stm][prevPiece][prevTo][piece][to] (continuation history)

void resetHistory() {
    memset(score_history, 0, sizeof(score_history));
    memset(cont_hist, 0, sizeof(cont_hist));
}

void updateScoreHistory(int depth, Side stm, Move move, const MoveList& quiets_tried) {
    const int bonus = std::min(depth * depth, MAX_HISTORY);
    auto& h = score_history[stm][From(move)][To(move)];
    h += bonus - (h * std::abs(bonus) / MAX_HISTORY);

    // malus: penalize all quiet moves searched before this cutoff
    for (const Move quiet_move : quiets_tried) {
        if (quiet_move == move) { // We just gave this move a bonus
            continue;
        }

        auto& h = score_history[stm][From(quiet_move)][To(quiet_move)];
        h += -bonus - (h * std::abs(bonus) / MAX_HISTORY);
    }
}

int getScoreHistory(Side stm, Move move) {
    return score_history[stm][From(move)][To(move)];
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