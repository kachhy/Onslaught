#include "search.h"
#include "core/move.h"
#include "eval.h"
#include "hash/transposition.h"
#include "movegen/movegen.h"
#include "terms.h"

int quiesce(Board& board, int alpha, int beta) {
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
    return board.score_history[MovePiece(move)][To(move)];
}

int search(Board& board, int depth, int alpha, int beta, int ply) {
    if (depth == 0) {
        return quiesce(board, alpha, beta);
    }

    // Transposition table
    Entry tt_entry;
    bool tt_hit = tt.fetch(board, tt_entry);

    if (tt_hit && tt_entry.depth >= static_cast<size_t>(depth)) {
        if (tt_entry.bound == EXACTBOUND || (tt_entry.bound == LOWERBOUND && tt_entry.score >= beta) || (tt_entry.bound == UPPERBOUND && tt_entry.score <= alpha)) {
            return tt_entry.score;
        }
    }

    MoveList moves = getLegalMoves(board);
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
        int score = -search(board, depth - 1, -beta, -alpha, ply + 1);
        board.undoMove(move);

        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score > alpha) {
                alpha = score;
            }
        }
        if (score >= beta) {
            tt.insert(board, best_move, best_score, LOWERBOUND, depth);
            if (!Capture(best_move) && !Prom(best_move)) {
                board.killers[ply][1] = board.killers[ply][0];
                board.killers[ply][0] = best_move;
                board.score_history[MovePiece(best_move)][To(best_move)] += depth * depth;
            }
            return best_score; // Fail-soft beta
        }
    }

    TTBound bound = (best_score > original_alpha) ? EXACTBOUND : UPPERBOUND;
    tt.insert(board, best_move, best_score, bound, depth);

    return best_score;
}

Move search(Board& board, int max_depth, int& best_score) {
    Move best_move = NO_MOVE;
    for (int depth = 1; depth <= max_depth; depth++) {
        best_score = -SCORE_MAX;
        for (int (&history_piece)[64] : board.score_history) {
            for (int& score : history_piece) {
                score /= 2;
            }
        }
        int score = -SCORE_MAX;
        int alpha = -SCORE_MAX;
        int beta = SCORE_MAX;
        Move cur_iteration_best = NO_MOVE;

        MoveList moves = getLegalMoves(board);
        std::array<int, MAX_MOVES> scores;
        for (uint8_t i = 0; i < moves.size(); i++) {
            scores[i] = scoreMove(board, moves[i], NO_MOVE, 0);
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
            Move move = moves[i];
            board.makeMove(move);
            score = -search(board, depth - 1, -beta, -alpha, 1);
            board.undoMove(move);

            if (score > best_score) {
                best_score = score;
                cur_iteration_best = move;
                if (score > alpha) {
                    alpha = score;
                }
            }
            if (score >= beta) {
                break;
            }
        }
        if (cur_iteration_best != NO_MOVE) {
            best_move = cur_iteration_best;
        }
    }

    return best_move;
}