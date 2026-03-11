#include "search.h"

int search(Board& board, int depth, int alpha, int beta, int ply) {
    if (depth == 0) {
        return eval(board);
    }

    // Checkmate + draw detection goes here

    int best_score = -SCORE_MAX;
    MoveList moves = getLegalMoves(board);

    for (const Move move : moves) {
        board.makeMove(move);
        int score = -search(board, depth - 1, -beta, -alpha, ply + 1);
        board.undoMove(move);

        if (score > best_score) {
            best_score = score;
            if (score > alpha) {
                alpha = score;
            }
        }
        if (score >= beta) {
            return best_score; // Fail-soft beta
        }
    }

    return best_score;
}