#include "search.h"
#include "board/rules.h"
#include "core/bitboard.h"
#include "core/move.h"
#include "core/types.h"
#include "eval.h"
#include "hash/transposition.h"
#include "history.h"
#include "movegen/attacks.h"
#include "movegen/movegen.h"
#include "terms.h"
#include "uci/uci.h"
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>

thread_local uint16_t seldepth;
thread_local uint64_t nodes;

struct PVLine {
    uint16_t cur_move;
    Move moves[256];
};

int scoreToMateScore(int score) {
    if (score > 0) {
        return (SCORE_MAX - score + 1) / 2;
    } else {
        return -(SCORE_MAX + score + 1) / 2;
    }
}

int scoreToTT(int score, int ply) {
    if (score >= SCORE_MAX - MAX_GAME_MOVES) {
        return score + ply;
    }
    if (score <= -SCORE_MAX + MAX_GAME_MOVES) {
        return score - ply;
    }
    return score;
}

int scoreFromTT(int score, int ply) {
    if (score >= SCORE_MAX - MAX_GAME_MOVES) {
        return score - ply;
    }
    if (score <= -SCORE_MAX + MAX_GAME_MOVES) {
        return score + ply;
    }
    return score;
}

void printInfo(const Board& src, int depth, int seldepth, int score, const char* bound, long long nodes, int nps, PVLine* pv) {
    std::cout << "info depth " << depth << " seldepth " << seldepth;
    if (std::abs(score) < SCORE_MAX - MAX_GAME_MOVES) {
        std::cout << " score cp " << score;
    } else {
        std::cout << " score mate " << scoreToMateScore(score);
    }
    if (bound) {
        std::cout << " " << bound;
    }
    std::cout << " nodes " << nodes << " nps " << nps << " pv ";
    if (!bound) {
        for (uint16_t i = 0; i < pv[0].cur_move; i++) {
            std::cout << moveToStr(pv[0].moves[i]) << ' ';
        }
    } else { // Reconstruct from TT
        auto bp = std::make_unique<Board>(src);
        Board& board = *bp;
        for (int i = 0; i < MAX_PLY; i++) {
            Entry tt_entry;
            bool tt_hit = tt.fetch(board, tt_entry);

            if (!tt_hit || tt_entry.best_move == NO_MOVE) {
                break;
            }

            MoveList moves;
            getLegalMoves(board, moves);
            bool valid_move = false;
            for (const Move& move : moves) {
                if (move == tt_entry.best_move) {
                    valid_move = true;
                    break;
                }
            }

            if (valid_move) {
                std::cout << moveToStr(tt_entry.best_move) << ' ';
                board.makeMove(tt_entry.best_move);
                if (isDraw(board, i)) {
                    break;
                }
            } else {
                break;
            }
        }
    }
    std::cout << std::endl;
}
// SEE
int staticExchangeEval(const Board& board, Move move, int threshold) {
    if (Castle(move) || IsEP(move) || Prom(move)) {
        return 1;
    }

    Square from = From(move);
    Square to = To(move);

    int v = SEE_VALUES[makeDefaultPiece(board.pieceAt(to))] - threshold;
    if (v < 0) {
        return 0;
    }

    v = SEE_VALUES[makeDefaultPiece(MovePiece(move))] - v;
    if (v <= 0) {
        return 1;
    }

    int stm = board.getSTM();

    BitBoard occ = board.getOcc(BOTH) ^ (BitBoard(1) << from) ^ (BitBoard(1) << to);
    BitBoard attackers = getAttackers(board, to, occ);
    BitBoard mine, lowest_attacker;

    const BitBoard diag = board.getPieceBB(WHITE_BISHOP) | board.getPieceBB(BLACK_BISHOP) | board.getPieceBB(WHITE_QUEEN) | board.getPieceBB(BLACK_QUEEN);
    const BitBoard straight = board.getPieceBB(WHITE_ROOK) | board.getPieceBB(BLACK_ROOK) | board.getPieceBB(WHITE_QUEEN) | board.getPieceBB(BLACK_QUEEN);

    int result = 1;

    while (true) {
        stm ^= 1;
        attackers &= occ;

        if (!(mine = (attackers & board.getOcc(static_cast<Side>(stm))))) {
            break;
        }

        result ^= 1;

        int pt;
        for (pt = PAWN; pt <= QUEEN; pt++) {
            lowest_attacker = mine & board.getPieceBB(makePiece(static_cast<DefaultPiece>(pt), static_cast<Side>(stm)));
            if (lowest_attacker) {
                break;
            }
        }

        if (pt > QUEEN) {
            return (attackers & ~board.getOcc(static_cast<Side>(stm))) ? result ^ 1 : result;
        }

        if ((v = SEE_VALUES[pt] - v) < result) {
            break;
        }

        occ ^= (lowest_attacker & -lowest_attacker);
        if (pt == PAWN || pt == BISHOP || pt == QUEEN) {
            attackers |= getBishopAttacks(to, occ) & diag;
        }
        if (pt == ROOK || pt == QUEEN) {
            attackers |= getRookAttacks(to, occ) & straight;
        }
    }

    return result;
}

int quiesce(Board& board, int alpha, int beta, int ply, int qply) {
    if (ply >= seldepth) {
        seldepth = ply;
    }

    nodes++;

    if (ply >= MAX_PLY) {
        return eval(board);
    }

    if (ply > 0 && isDraw(board, ply)) {
        return 0;
    }

    int static_eval;
    int best_value;
    MoveList moves;

    if (board.inCheck()) {
        best_value = -SCORE_MAX + std::min(ply, (int)MAX_PLY - 1);
        getLegalMoves(board, moves);
    } else {
        static_eval = eval(board);
        best_value = static_eval;

        if (best_value >= beta) {
            return best_value;
        }

        if (best_value > alpha) {
            alpha = best_value;
        }

        getNoisyMoves(board, moves);
    }

    std::array<int, MAX_MOVES> scores;
    for (uint8_t i = 0; i < moves.size(); i++) {
        if (Capture(moves[i]) || IsEP(moves[i])) {
            DefaultPiece attacker = makeDefaultPiece(MovePiece(moves[i]));
            Piece victim_piece = IsEP(moves[i]) ? makePiece(PAWN, board.getXSTM()) : board.pieceAt(To(moves[i]));
            scores[i] = MVV_LVA[makeDefaultPiece(victim_piece)][attacker];
        } else {
            scores[i] = 0; // only here if in check
        }
    }

    for (uint8_t i = 0; i < moves.size(); i++) {
        uint8_t best_move_index = i;
        for (uint8_t j = i + 1; j < moves.size(); j++) {
            if (scores[j] > scores[best_move_index]) {
                best_move_index = j;
            }
        }

        moves.sort_item(best_move_index);
        std::swap(scores[i], scores[best_move_index]);
        Move noisy_move = moves[i];

        // Delta pruning
        if (Capture(noisy_move) && !Prom(noisy_move) && best_value + DELTA_PRUNING_MARGIN + SEE_VALUES[makeDefaultPiece(board.pieceAt(To(noisy_move)))] < alpha) { // Borrowing SEE values since they're very similar to raw material values
            continue;
        }

        // SEE pruning
        if (!board.inCheck() && !staticExchangeEval(board, noisy_move, 0)) {
            continue;
        }

        board.makeMove(noisy_move);
        int score = -quiesce(board, -beta, -alpha, ply + 1, qply + 1);
        board.undoMove(noisy_move);

        if (score >= beta) {
            return score;
        }
        if (score > best_value) {
            best_value = score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return best_value;
}

static int scoreMove(Board& board, Move move, Move tt_move, SearchStack* ss) {
    if (move == tt_move) {
        return 150000; // priority to existing best move
    }

    if (Capture(move) || IsEP(move)) {
        DefaultPiece attacker = makeDefaultPiece(MovePiece(move));
        Piece victim_piece = IsEP(move) ? makePiece(PAWN, board.getXSTM()) : board.pieceAt(To(move));
        DefaultPiece victim = makeDefaultPiece(victim_piece);
        return 130000 + MVV_LVA[victim][attacker];
    }

    if (Prom(move)) {
        return 120000;
    }

    if (move == ss->killers[0]) {
        return 110000;
    }
    if (move == ss->killers[1]) {
        return 100000;
    }

    int history = getScoreHistory(board.getSTM(), move) + getContHist(ss, board.getSTM(), move);

    return history;
}

int search(
    Board& board, int depth, int alpha, int beta, size_t hard_cap, long long max_nodes, std::chrono::high_resolution_clock::time_point start, int ply, SearchStack* ss,
    bool can_make_null_move, PVLine pv_table[], int max_ply
) {
    if (ply >= seldepth) {
        seldepth = ply;
    }
    if (ply >= MAX_PLY) {
        return eval(board);
    }

    pv_table[ply].cur_move = 0;

    if (!(nodes & 4095)) {
        checkStdin(start, max_nodes, nodes, hard_cap);
        if (!searching) {
            return eval(board);
        }
    }

    if (ply > 0 && isDraw(board, ply)) {
        return 0;
    }
    // mdp
    alpha = std::max(alpha, -SCORE_MAX + ply);
    beta = std::min(beta, SCORE_MAX - ply - 1);
    if (alpha >= beta) {
        return alpha;
    }
    // quiesce on depth <= 0

    if (depth <= 0) {
        pv_table[ply].cur_move = 0;
        return quiesce(board, alpha, beta, ply + 1, 0);
    }

    nodes++;
    bool is_pv = beta - alpha != 1;

    // find if this position has already been searched at a good depth and returns its score
    Entry tt_entry;
    bool tt_hit = tt.fetch(board, tt_entry);
    if (tt_hit) {
        tt_entry.score = scoreFromTT(tt_entry.score, ply);

        if (!is_pv && ply > 0 && ss->excluded == NO_MOVE && tt_entry.depth >= static_cast<size_t>(depth)) {
            if (tt_entry.bound == EXACTBOUND || (tt_entry.bound == LOWERBOUND && tt_entry.score >= beta) || (tt_entry.bound == UPPERBOUND && tt_entry.score <= alpha)) {
                return tt_entry.score;
            }
        }
    }

    bool in_check = board.inCheck();
    if (in_check) {
        depth++; // check extension
    }
    if (in_check) { // important; this prevents the improving flag from being false after check sequence finsishes
        ss->static_eval = (ply >= 2 ? (ss - 2)->static_eval : 0);
    } else {
        if (tt_hit) {
            if (tt_entry.bound == EXACTBOUND) {
                ss->static_eval = tt_entry.score;
            } else {
                ss->static_eval = eval(board);
                if (tt_entry.bound == (tt_entry.score > ss->static_eval ? LOWERBOUND : UPPERBOUND)) {
                    ss->static_eval = tt_entry.score;
                }
            }
        } else {
            ss->static_eval = eval(board);
        }
    }

    int static_eval = ss->static_eval;

    // imrpoving checks
    // bool improving = !in_check && ply >= 2 && static_eval > board.static_evals[ply - 2];

    // rfp (prune worse positions harder with improving position)
    // TODO Tune the rfp margin constant
    if (!is_pv && !in_check && depth <= 6 && static_eval - RFP_MARGIN * (depth /* - (improving && depth > 1)*/) >= beta) {
        return static_eval;
    }

    // nmp
    BitBoard non_pawn_material = board.getOcc(board.getSTM()) & ~board.getPieceBB(makePiece(PAWN, board.getSTM())) & ~board.getPieceBB(makePiece(KING, board.getSTM()));
    if (!is_pv && can_make_null_move && !in_check && depth >= NMP_DEPTH_CUTOFF && static_eval >= beta && non_pawn_material) {
        int nmp_reduction = 3 + depth / 6;
        ss->move = NO_MOVE;
        board.makeNullMove();
        tt.prefetch(board.hash());
        int score = -search(board, depth - 1 - nmp_reduction, -beta, -beta + 1, hard_cap, max_nodes, start, ply + 1, ss + 1, false, pv_table, max_ply);
        board.undoNullMove();

        if (score >= SCORE_MAX - MAX_GAME_MOVES) { // Prevent including mate scores
            score = beta;
        }

        if (score >= beta) {
            return score;
        }
    }

    // razoring = save movegen cost on pruned nodes by checking if it can beat alpha with quiesce, otherwise fail low
    if (!is_pv && !in_check && depth <= RAZORING_DEPTH_MAX && static_eval + RAZOR_MARGIN * depth < alpha) {
        int quiescent_score = quiesce(board, alpha - 1, alpha, ply + 1, 0);
        if (quiescent_score < alpha) {
            return quiescent_score;
        }
    }

    // iir (no tt move)
    if (depth >= IIR_DEPTH_CUTOFF && (!tt_hit || tt_entry.best_move == NO_MOVE)) {
        depth--;
    }

    MoveList moves;
    getLegalMoves(board, moves);
    if (moves.size() == 0) {
        pv_table[ply].cur_move = 0;
        return in_check ? -SCORE_MAX + ply : 0; // checkmate or stalemate
    }

    std::array<int, MAX_MOVES> scores;
    for (uint8_t i = 0; i < moves.size(); i++) {
        scores[i] = scoreMove(board, moves[i], tt_hit ? tt_entry.best_move : NO_MOVE, ss);
    }

    int original_alpha = alpha;
    int best_score = -SCORE_MAX;
    Move best_move = NO_MOVE;

    // PVS
    int moves_searched = 0;

    // history malus. apply penalties when all moves fail low
    MoveList quiets_tried;

    // futility pruning
    bool futility_pruning = !is_pv && !in_check && depth <= FP_DEPTH_MAX && static_eval + FUTILITY_MARGIN * depth <= alpha;

    for (uint8_t i = 0; i < moves.size(); i++) {
        // move ordering
        uint8_t best_move_index = i;
        for (uint8_t j = i + 1; j < moves.size(); j++) {
            if (scores[j] > scores[best_move_index]) {
                best_move_index = j;
            }
        }
        moves.sort_item(best_move_index);
        std::swap(scores[i], scores[best_move_index]);

        Move move = moves[i];
        if (move == ss->excluded) {
            continue;
        }

        bool is_quiet_move = !Capture(move) && !Prom(move);
        bool gives_check = givesCheck(board, move);
        // futility pruning: if static_eval + margin <= alpha, prune quiet moves bc they are unlikely to improve position
        if (futility_pruning && moves_searched > 0 && is_quiet_move && !gives_check) {
            continue;
        }

        // late move pruning: at low depth, once enough moves are tried, skip the remaining
        // quiets. Relies on move ordering placing good quiets early.
        if (!is_pv && !in_check && is_quiet_move && !gives_check && depth <= LMP_DEPTH_MAX && moves_searched >= LMP_BASE + depth * depth) {
            continue;
        }

        if (!in_check && moves_searched > 0 && Capture(move) && depth <= SEE_DEPTH_MAX && !staticExchangeEval(board, move, -20 * depth * depth)) {
            continue;
        }

        if (is_quiet_move) {
            quiets_tried.emplace_back(move);
        }

        // singular extensions: if the TT move is much better than all alternatives
        // (a reduced search of the other moves fails low below a margin), extend it.
        int extension = 0;
        if (ply > 0 && ss->excluded == NO_MOVE && depth >= 8 && tt_hit && move == tt_entry.best_move && tt_entry.bound != UPPERBOUND &&
            tt_entry.depth >= static_cast<size_t>(depth) - 3 && std::abs(tt_entry.score) < SCORE_MAX - MAX_GAME_MOVES) {
            int s_beta = tt_entry.score - 2 * depth;
            ss->excluded = move;
            int s_score = search(board, (depth - 1) / 2, s_beta - 1, s_beta, hard_cap, max_nodes, start, ply, ss, can_make_null_move, pv_table, max_ply);
            ss->excluded = NO_MOVE;

            if (s_score < s_beta) {
                extension = 1; // singular
                if (!is_pv && s_score < s_beta - 20) {
                    extension = 2; // Double extension
                }
            } else if (s_beta >= beta) { // Multi-cut pruning
                return s_beta;
            } else if (tt_entry.score >= beta) {
                extension = -1; // negative extension
            }
        }

        ss->move = move;
        board.makeMove(move);
        tt.prefetch(board.hash());

        int score;
        int new_depth = depth - 1 + extension;
        bool do_full_search = false;

        if (moves_searched == 0) {
            score = -search(board, new_depth, -beta, -alpha, hard_cap, max_nodes, start, ply + 1, ss + 1, true, pv_table, max_ply);
        } else {
            // lmr
            if (moves_searched >= LMR_MOVES_CUTOFF && depth >= LMR_DEPTH_CUTOFF && !Capture(move) && !Prom(move) && !in_check) {
                // TODO tune this function
                // improving flag = search more carefully when good position is improving (less reduction)
                int lmr_reduction = std::max(0, std::min((int)(LMR_VALUE + (log(depth)) * log(moves_searched) / LMR_SCALAR), depth - 2) /* - improving*/);
                // history-based reduction: reduce good-history quiets less, bad-history more.
                const int move_hist = getScoreHistory(board.getXSTM(), move) + getContHist(ss, board.getXSTM(), move);
                lmr_reduction = std::max(0, std::min(lmr_reduction - move_hist / HIST_LMR_DIVISOR, depth - 2));
                score = -search(board, new_depth - lmr_reduction, -alpha - 1, -alpha, hard_cap, max_nodes, start, ply + 1, ss + 1, true, pv_table, max_ply);
                do_full_search = score > alpha;
            } else {
                do_full_search = true;
            }

            // pvs at full depth
            if (do_full_search) {
                score = -search(board, new_depth, -alpha - 1, -alpha, hard_cap, max_nodes, start, ply + 1, ss + 1, true, pv_table, max_ply);
                if (score > alpha && score < beta) {
                    score = -search(board, new_depth, -beta, -alpha, hard_cap, max_nodes, start, ply + 1, ss + 1, true, pv_table, max_ply);
                }
            }
        }

        board.undoMove(move);
        moves_searched++;

        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score > alpha) {
                alpha = score;
                // Update PV for this ply from the pre-allocated table
                pv_table[ply].moves[0] = move;
                memcpy(pv_table[ply].moves + 1, pv_table[ply + 1].moves, pv_table[ply + 1].cur_move * sizeof(Move));
                pv_table[ply].cur_move = pv_table[ply + 1].cur_move + 1;
            }
        }
        if (score >= beta) {
            if (ss->excluded == NO_MOVE) {
                tt.insert(board, best_move, scoreToTT(best_score, ply), LOWERBOUND, depth);
            }

            if (!Capture(best_move) && !Prom(best_move)) {
                if (best_move != ss->killers[0]) { // Update killer moves
                    ss->killers[1] = ss->killers[0];
                    ss->killers[0] = best_move;
                }

                updateScoreHistory(depth, board.getSTM(), best_move, quiets_tried);
                if (ply >= 1) {
                    updateContHistory(depth, board.getSTM(), best_move, ss, quiets_tried);
                }
            }

            return best_score;
        }
    }

    // If only the excluded move was available, fail low (move is singular)
    if (ss->excluded != NO_MOVE && best_score == -SCORE_MAX) {
        return alpha;
    }

    TTBound bound = (best_score > original_alpha) ? EXACTBOUND : UPPERBOUND;
    if (ss->excluded == NO_MOVE) {
        tt.insert(board, best_move, scoreToTT(best_score, ply), bound, depth);
    }
    
    return best_score;
}

Move search(Board& board, int max_depth, int& best_score, const GoParams& params) {
    Move best_move = NO_MOVE;
    PVLine pv_table[MAX_PLY + 1]; // pre-allocated, indexed by ply
    nodes = 0;
    best_score = 0;

    // Setup search stack. Pad the front by 4 so continuation history can read
    // (ss - 1/2/4) at low plies without underflowing; padding is zero-init (NO_MOVE).
    SearchStack sstack_buf[MAX_PLY + 5] = {};
    SearchStack* sstack = sstack_buf + 4;

    // Reset history
    resetHistory();

    auto start = std::chrono::high_resolution_clock::now();
    size_t hard_cap, soft_cap;

    if (params.movetime != -1) {
        hard_cap = std::max(params.movetime, 1);
        soft_cap = params.movetime;
    } else if (board.getSTM() == WHITE) {
        if (params.wtime == -1) {
            hard_cap = 0;
            soft_cap = 0;
        } else {
            if (params.movestogo > 0) {
                hard_cap = std::max(params.wtime / params.movestogo + params.winc, 1);
                soft_cap = params.wtime / params.movestogo + params.winc;
            } else {
                hard_cap = std::max(params.wtime / 20 + params.winc / 2, 1);
                soft_cap = params.wtime / 30 + params.winc / 3;
            }
        }
    } else {
        if (params.btime == -1) {
            hard_cap = 0;
            soft_cap = 0;
        } else {
            if (params.movestogo > 0) {
                hard_cap = std::max(params.btime / params.movestogo + params.binc, 1);
                soft_cap = params.btime / params.movestogo + params.binc;
            } else {
                hard_cap = std::max(params.btime / 20 + params.binc / 2, 1);
                soft_cap = params.btime / 30 + params.binc / 3;
            }
        }
    }

    for (int depth = 1; depth <= max_depth; depth++) {
        if (!searching) {
            break;
        }
        seldepth = 0;
        tt.incAge();

        // aspiration window
        int delta = ASPIRATION_MARGIN;
        int alpha;
        int beta;
        if (depth >= ASPIRATION_DEPTH_CUTOFF) {
            alpha = best_score - delta;
            beta = best_score + delta;
        } else {
            alpha = -SCORE_MAX;
            beta = SCORE_MAX;
        }

        int iter_score = best_score;

        while (true) {
            // Clear PV table before each search
            for (int i = 0; i <= MAX_PLY; i++) {
                pv_table[i].cur_move = 0;
            }

            iter_score = search(board, depth, alpha, beta, hard_cap, params.nodes, start, 0, sstack, true, pv_table, MAX_PLY);
            if (!searching) {
                break;
            }

            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            int nps = duration.count() == 0 ? nodes : static_cast<int>((double)nodes / (duration.count() / 1000.0));

            if (iter_score <= alpha) {
                // fail low = true score is at most alpha (upper bound)
                if (!params.silent) {
                    printInfo(board, depth, seldepth, iter_score, "upperbound", nodes, nps, pv_table);
                }
                alpha = std::max(-SCORE_MAX, iter_score - delta);
                delta *= (1 + ASPIRATION_SCALAR);
            } else if (iter_score >= beta) {
                // fail high = true score is at least beta (lower bound)
                if (!params.silent) {
                    printInfo(board, depth, seldepth, iter_score, "lowerbound", nodes, nps, pv_table);
                }
                beta = std::min(SCORE_MAX, iter_score + delta);
                delta *= (1 + ASPIRATION_SCALAR);
            } else {
                best_score = iter_score;
                if (!params.silent) {
                    printInfo(board, depth, seldepth, best_score, nullptr, nodes, nps, pv_table);
                }
                if (pv_table[0].cur_move > 0) {
                    best_move = pv_table[0].moves[0];
                } else { // Fallback to TT for best move
                    Entry tt_entry;
                    if (tt.fetch(board, tt_entry) && tt_entry.best_move != NO_MOVE) {
                        best_move = tt_entry.best_move;
                    }
                }
                break;
            }
        }
        if (!searching) {
            break;
        }
        if (soft_cap != 0 && (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= soft_cap) {
            break;
        }
    }

    return best_move;
}