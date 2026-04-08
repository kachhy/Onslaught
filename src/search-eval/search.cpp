#include "search.h"
#include "core/move.h"
#include "eval.h"
#include "hash/transposition.h"
#include "movegen/movegen.h"
#include "uci/uci.h"
#include "terms.h"
#include <array>
#include <chrono>
#include <iostream>

uint16_t seldepth;
uint64_t nodes;

struct PVLine {
    uint16_t cur_move;
    Move moves[256];
};

int quiesce(Board& board, int alpha, int beta) {
    nodes++;
    int static_eval;
    int best_value;
    MoveList moves;
    if (board.inCheck()) {
        best_value = -SCORE_MAX;
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
        int score = -quiesce(board, -beta, -alpha);
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

int search(Board& board, int depth, int alpha, int beta, int ply, PVLine pv_table[], int max_ply) {
    if (ply >= seldepth) {
        seldepth = ply;
    }

    if (!(nodes & 4095)) {
        checkStdin();
        if (!searching) {
            return eval(board);
        }
    }
    
    if (depth == 0) {
        pv_table[ply].cur_move = 0;
        return quiesce(board, alpha, beta);
    }

    nodes++;

    Entry tt_entry;
    bool tt_hit = tt.fetch(board, tt_entry);

    if (tt_hit && tt_entry.depth >= static_cast<size_t>(depth)) {
        if (tt_entry.bound == EXACTBOUND || (tt_entry.bound == LOWERBOUND && tt_entry.score >= beta) || (tt_entry.bound == UPPERBOUND && tt_entry.score <= alpha)) {
            return tt_entry.score;
        }
    }

    MoveList moves = getLegalMoves(board);
    if (moves.size() == 0) {
        pv_table[ply].cur_move = 0;
        return board.inCheck() ? -SCORE_MAX + ply : 0; // checkmate or stalemate
    }

    std::array<int, MAX_MOVES> scores;
    for (uint8_t i = 0; i < moves.size(); i++) {
        scores[i] = scoreMove(board, moves[i], tt_hit ? tt_entry.best_move : NO_MOVE, ply);
    }

    int original_alpha = alpha;
    int best_score = -SCORE_MAX;
    Move best_move = NO_MOVE;

    for (uint8_t i = 0; i < moves.size(); i++) {
        uint8_t best_move_index = i;
        for (uint8_t j = i + 1; j < moves.size(); j++) {
            if (scores[j] > scores[best_move_index]) {
                best_move_index = j;
            }
        }
        moves.sort_item(best_move_index);
        std::swap(scores[i], scores[best_move_index]);

        Move move = moves[i];
        board.makeMove(move);
        int score = -search(board, depth - 1, -beta, -alpha, ply + 1, pv_table, max_ply);
        board.undoMove(move);

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
                board.score_history[To(best_move)][MovePiece(best_move)] += depth * depth;
            }
            return best_score;
        }
    }

    TTBound bound = (best_score > original_alpha) ? EXACTBOUND : UPPERBOUND;
    tt.insert(board, best_move, best_score, bound, depth);
    return best_score;
}

Move search(Board& board, int max_depth, int& best_score) {
    Move best_move = NO_MOVE;
    const int MAX_PLY = 128;
    PVLine pv_table[MAX_PLY + 1]; // pre-allocated, indexed by ply
    nodes = 0;

    for (int depth = 1; depth <= max_depth; depth++) {
        if (!searching) {
            break;
        }
        seldepth = 0;
        best_score = -SCORE_MAX;

        for (int (&history_score)[12] : board.score_history) {
            for (int& score : history_score) {
                score /= 2;
            }
        }

        // Clear PV table for this iteration
        for (int i = 0; i <= MAX_PLY; i++) {
            pv_table[i].cur_move = 0;
        }

        tt.incAge();

        int alpha = -SCORE_MAX;
        int beta = SCORE_MAX;
        Move cur_iteration_best = NO_MOVE;

        MoveList moves = getLegalMoves(board);
        std::array<int, MAX_MOVES> scores;
        for (uint8_t i = 0; i < moves.size(); i++) {
            scores[i] = scoreMove(board, moves[i], best_move, 0);
        }

        auto start = std::chrono::high_resolution_clock::now();
        for (uint8_t i = 0; i < moves.size(); i++) {
            uint8_t best_move_index = i;
            for (uint8_t j = i + 1; j < moves.size(); j++) {
                if (scores[j] > scores[best_move_index]) {
                    best_move_index = j;
                }
            }
            moves.sort_item(best_move_index);
            std::swap(scores[i], scores[best_move_index]);

            Move move = moves[i];
            board.makeMove(move);
            int score = -search(board, depth - 1, -beta, -alpha, 1, pv_table, MAX_PLY);
            board.undoMove(move);
            if (!searching) {
                break;
            }

            if (score > best_score) {
                best_score = score;
                cur_iteration_best = move;
                // Root is ply 0, build PV the same way
                pv_table[0].moves[0] = move;
                memcpy(pv_table[0].moves + 1, pv_table[1].moves, pv_table[1].cur_move * sizeof(Move));
                pv_table[0].cur_move = pv_table[1].cur_move + 1;

                if (score > alpha) {
                    alpha = score;
                }
            }
            if (score >= beta) {
                break;
            }
        }
        if (!searching) {
            break;
        }
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

        if (cur_iteration_best != NO_MOVE) {
            best_move = cur_iteration_best;
        }

        std::cout << "info depth " << depth << " seldepth " << seldepth << " score cp " << best_score << " nodes " << nodes << " nps "
                  << (!duration.count() ? nodes : static_cast<int>(static_cast<double>(nodes) / (static_cast<double>(duration.count()) / 1000))) << " pv ";
        for (uint16_t i = 0; i < pv_table[0].cur_move; i++) {
            std::cout << moveToStr(pv_table[0].moves[i]) << ' ';
        }
        std::cout << std::endl;
    }

    return best_move;
}