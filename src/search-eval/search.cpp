#include "search.h"

int search(Board& board, int depth, int alpha, int beta, int ply) {
    if (depth == 0) {
        return eval(board);
    }

    // Transposition table
    Entry tt_entry;
    bool tt_hit = tt.fetch(board, tt_entry);

    if (tt_hit && tt_entry.depth >= static_cast<size_t>(depth)) {
        if (tt_entry.bound == EXACTBOUND
            || (tt_entry.bound == LOWERBOUND && tt_entry.score >= beta)
            || (tt_entry.bound == UPPERBOUND && tt_entry.score <= alpha)) {
            return tt_entry.score;
        }
    }

    MoveList moves = getLegalMoves(board);
    int original_alpha = alpha;
    int best_score = -SCORE_MAX;
    Move best_move = NO_MOVE;

    for (const Move move : moves) {
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
            return best_score; // Fail-soft beta
        }
    }

    TTBound bound = (best_score > original_alpha) ? EXACTBOUND : UPPERBOUND;
    tt.insert(board, best_move, best_score, bound, depth);

    return best_score;
}

Move search(Board& board, int depth, int& best_score) {
    if (depth <= 0) {
        best_score = 0;
        return NO_MOVE;
    }
    int alpha = -SCORE_MAX;
    int beta = SCORE_MAX;
    Move best_move = NO_MOVE;
    best_score = -SCORE_MAX;

    MoveList moves = getLegalMoves(board);
    for (const Move move : moves) {
        board.makeMove(move);
        int score = -search(board, depth - 1, -beta, -alpha, 1);
        board.undoMove(move);

        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score > alpha) {
                alpha = score;
            }
        }
        if (score >= beta) {
            return best_move; // Fail-soft beta
        }
    }

    return best_move;
}