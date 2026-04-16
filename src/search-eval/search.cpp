#include "search.h"
#include "board/rules.h"
#include "core/move.h"
#include "core/types.h"
#include "eval.h"
#include "hash/transposition.h"
#include "movegen/movegen.h"
#include "terms.h"
#include "uci/uci.h"
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>

uint16_t seldepth;
uint64_t nodes;

struct PVLine {
    uint16_t cur_move;
    Move moves[256];
};

void print_info(int depth, int seldepth, int score, const char* bound, long long nodes, int nps, PVLine* pv) {
    std::cout << "info depth " << depth << " seldepth " << seldepth << " score cp " << score;
    if (bound) {
        std::cout << " " << bound;
    }
    std::cout << " nodes " << nodes << " nps " << nps << " pv ";
    for (uint16_t i = 0; i < pv[0].cur_move; i++) {
        std::cout << moveToStr(pv[0].moves[i]) << ' ';
    }
    std::cout << std::endl;
}

int quiesce(Board& board, int alpha, int beta, int ply, int qply) {
    if (ply >= seldepth) {
        seldepth = ply;
    }
    nodes++;
    int static_eval;
    int best_value;
    MoveList moves;
    if (board.inCheck()) {
        best_value = -SCORE_MAX + ply;
        moves = getLegalMoves(board);
    } else {
        static_eval = eval(board);
        best_value = static_eval;
        if (best_value >= beta) {
            return best_value;
        }
        if (best_value > alpha) {
            alpha = best_value;
        }
        moves = getNoisyMoves(board);
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

static int scoreMove(Board& board, Move move, Move tt_move, int ply) {
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

    if (move == board.killers[ply][0]) {
        return 110000;
    }
    if (move == board.killers[ply][1]) {
        return 100000;
    }
    return board.score_history[To(move)][MovePiece(move)];
}

int search(Board& board, int depth, int alpha, int beta, size_t hard_cap, long long max_nodes, std::chrono::high_resolution_clock::time_point start, int ply, bool can_make_null_move, PVLine pv_table[], int max_ply) {
    if (ply >= seldepth) {
        seldepth = ply;
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
    // find if this position has already been searched at a good depth and returns its score
    Entry tt_entry;
    bool tt_hit = tt.fetch(board, tt_entry);

    if (ply > 0 && tt_hit && tt_entry.depth >= static_cast<size_t>(depth)) {
        if (tt_entry.bound == EXACTBOUND || (tt_entry.bound == LOWERBOUND && tt_entry.score >= beta) || (tt_entry.bound == UPPERBOUND && tt_entry.score <= alpha)) {
            return tt_entry.score;
        }
    }

    bool is_pv = beta - alpha != 1;
    // // iir (no tt move)
    // if (!is_pv && depth >= 4 && (!tt_hit || tt_entry.best_move == NO_MOVE)) {
    //     depth--;
    // }

    bool in_check = board.inCheck();
    if (in_check) {
        depth++; // check extension
    }
    if (in_check) { // important; this prevents the improving flag from being false after check sequence finsishes
        board.static_evals[ply] = (ply >= 2 ? board.static_evals[ply - 2] : 0);
    } else {
        board.static_evals[ply] = eval(board);
    }

    int static_eval = board.static_evals[ply];

    // imrpoving checks
    // bool improving = !in_check && ply >= 2 && static_eval > board.static_evals[ply - 2];

    // rfp (prune worse positions harder with improving position)
    // TODO Tune the rfp margin constant
    if (!is_pv && !in_check && depth <= 6 && static_eval - RFP_MARGIN * (depth/* - (improving && depth > 1)*/) >= beta) {
        return static_eval;
    }

    // nmp
    BitBoard non_pawn_material = board.getOcc(board.getSTM()) & ~board.getPieceBB(makePiece(PAWN, board.getSTM())) & ~board.getPieceBB(makePiece(KING, board.getSTM()));
    if (!is_pv && can_make_null_move && !in_check && depth >= 3 && static_eval >= beta && non_pawn_material) {
        int nmp_reduction = 3 + depth / 6;
        board.makeNullMove();
        int score = -search(board, depth - 1 - nmp_reduction, -beta, -beta + 1, hard_cap, max_nodes, start, ply + 1, false, pv_table, max_ply);
        board.undoNullMove();
        if (score >= beta) {
            return score;
        }
    }

    // // razoring = save movegen cost on pruned nodes by checking if it can beat alpha with quiesce, otherwise fail low
    // if (!is_pv && !in_check && depth <= 3 && static_eval + RAZOR_MARGIN * depth < alpha) {
    //     int quiescent_score = quiesce(board, alpha - 1, alpha, ply + 1, 0);
    //     if (quiescent_score < alpha) {
    //         return quiescent_score;
    //     }
    // }

    MoveList moves = getLegalMoves(board);
    if (moves.size() == 0) {
        pv_table[ply].cur_move = 0;
        return in_check ? -SCORE_MAX + ply : 0; // checkmate or stalemate
    }

    std::array<int, MAX_MOVES> scores;
    for (uint8_t i = 0; i < moves.size(); i++) {
        scores[i] = scoreMove(board, moves[i], tt_hit ? tt_entry.best_move : NO_MOVE, ply);
    }

    int original_alpha = alpha;
    int best_score = -SCORE_MAX;
    Move best_move = NO_MOVE;

    // PVS
    int moves_searched = 0;

    // history malus. apply penalties when all moves fail low
    // MoveList quiets_tried;
    // int quiets_tried_count = 0;

    // // futility pruning
    // bool futility_pruning = !is_pv && !in_check && depth <= 5 && static_eval + FUTILITY_MARGIN * depth <= alpha;

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
        // bool is_quiet_move = !Capture(move) && !Prom(move);
        // bool gives_check = givesCheck(board, move);
        // // futility pruning: if static_eval + margin <= alpha, prune quiet moves bc they are unlikely to improve position
        // if (futility_pruning && moves_searched > 0 && is_quiet_move && !gives_check) {
        //     continue;
        // }
        // if (is_quiet_move) {
        //     quiets_tried[quiets_tried_count++] = move;
        // }
        board.makeMove(move);

        // // mate extensions (replaced by check extension at top of search)
        // int extension = 0;
        // if (gives_check && depth <= 2) {
        //     extension = 1;
        // }

        int score;
        bool do_full_search = false;

        if (moves_searched == 0) {
            score = -search(board, depth - 1, -beta, -alpha, hard_cap, max_nodes, start, ply + 1, true, pv_table, max_ply);
        } else {
            // lmr
            if (moves_searched >= 3 && depth >= 3 && !Capture(move) && !Prom(move) && !in_check) {
                // TODO tune this function
                // improving flag = search more carefully when good position is improving (less reduction)
                int lmr_reduction = std::max(0, std::min((int)(LMR_VALUE + (log(depth)) * log(moves_searched) / LMR_SCALAR), depth - 2)/* - improving*/);
                score = -search(board, depth - 1 - lmr_reduction, -alpha - 1, -alpha, hard_cap, max_nodes, start, ply + 1, true, pv_table, max_ply);
                do_full_search = score > alpha;
            } else {
                do_full_search = true;
            }
            // pvs at full depth
            if (do_full_search) {
                score = -search(board, depth - 1, -alpha - 1, -alpha, hard_cap, max_nodes, start, ply + 1, true, pv_table, max_ply);
                if (score > alpha && score < beta) {
                    score = -search(board, depth - 1, -beta, -alpha, hard_cap, max_nodes, start, ply + 1, true, pv_table, max_ply);
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
            tt.insert(board, best_move, best_score, LOWERBOUND, depth);
            if (!Capture(best_move) && !Prom(best_move)) {
                board.killers[ply][1] = board.killers[ply][0];
                board.killers[ply][0] = best_move;
                board.score_history[To(best_move)][MovePiece(best_move)] =
                    std::min(board.score_history[To(best_move)][MovePiece(best_move)] + depth * depth, MAX_HISTORY);
                // malus: penalize all quiet moves searched before this cutoff
                // for (int j = 0; j < quiets_tried_count - 1; j++) {
                //     Move m = quiets_tried[j];
                //     board.score_history[To(m)][MovePiece(m)] = std::max(board.score_history[To(m)][MovePiece(m)] - depth * depth, -MAX_HISTORY);
                // }
            }
            return best_score;
        }
    }

    TTBound bound = (best_score > original_alpha) ? EXACTBOUND : UPPERBOUND;
    // history malus for when no beta cutoffs
    // if (bound == UPPERBOUND) {
    //     for (int j = 0; j < quiets_tried_count; j++) {
    //         Move m = quiets_tried[j];
    //         board.score_history[To(m)][MovePiece(m)] = std::max(board.score_history[To(m)][MovePiece(m)] - depth * depth, -MAX_HISTORY);
    //     }
    // }
    tt.insert(board, best_move, best_score, bound, depth);
    return best_score;
}

Move search(Board& board, int max_depth, int& best_score, const GoParams& params) {
    Move best_move = NO_MOVE;
    const int MAX_PLY = 128;
    PVLine pv_table[MAX_PLY + 1]; // pre-allocated, indexed by ply
    nodes = 0;
    best_score = 0;

    auto start = std::chrono::high_resolution_clock::now();
    size_t hard_cap, soft_cap;

    if(board.getSTM() == WHITE){
        if(params.wtime == -1) {
            hard_cap = 0;
            soft_cap = 0;
        }
        else{
            hard_cap = params.wtime / 20 + params.winc / 2;
            soft_cap = params.wtime / 30 + params.winc / 3;
        }
    }
    else{
        if(params.btime == -1) {
            hard_cap = 0;
            soft_cap = 0;
        }
        else {
            hard_cap = params.btime / 20 + params.binc / 2;
            soft_cap = params.btime / 30 + params.binc / 3;
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
        if (depth >= 5) {
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

            iter_score = search(board, depth, alpha, beta, hard_cap, params.nodes, start, 0, true, pv_table, MAX_PLY);
            if (!searching) {
                break;
            }

            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            int nps = duration.count() == 0 ? nodes : static_cast<int>((double)nodes / (duration.count() / 1000.0));

            if (iter_score <= alpha) {
                // fail low = true score is at most alpha (upper bound)
                print_info(depth, seldepth, iter_score, "upperbound", nodes, nps, pv_table);
                alpha = std::max(-SCORE_MAX, iter_score - delta);
                delta *= 2;
            } else if (iter_score >= beta) {
                // fail high = true score is at least beta (lower bound)
                print_info(depth, seldepth, iter_score, "lowerbound", nodes, nps, pv_table);
                beta = std::min(SCORE_MAX, iter_score + delta);
                delta *= 2;
            } else {
                best_score = iter_score;
                print_info(depth, seldepth, best_score, nullptr, nodes, nps, pv_table);
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
        for (int (&history_score)[12] : board.score_history) {
            for (int& score : history_score) {
                score /= 2;
            }
        }
        if (!searching) {
            break;
        }
        if(soft_cap != 0 && (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(start - std::chrono::high_resolution_clock::now()).count() >= soft_cap){
            break;
        }
    }

    return best_move;
}
