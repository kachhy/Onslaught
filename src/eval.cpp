#include "eval.h"

int material_values[6] = {
    100, // PAWN
    300, // KNIGHT
    300, // BISHOP
    500, // ROOK
    900, // QUEEN
    0, // KING (shouldn't hit this case ever)
};

int eval(const Board& board) {
    int score = 0;

    score += material_values[PAWN] * bitCount(board.getPieceBB(WHITE_PAWN));
    score += material_values[KNIGHT] * bitCount(board.getPieceBB(WHITE_KNIGHT));
    score += material_values[BISHOP] * bitCount(board.getPieceBB(WHITE_BISHOP));
    score += material_values[ROOK] * bitCount(board.getPieceBB(WHITE_ROOK));
    score += material_values[QUEEN] * bitCount(board.getPieceBB(WHITE_QUEEN));

    score += material_values[PAWN] * -1 * bitCount(board.getPieceBB(BLACK_PAWN));
    score += material_values[KNIGHT] * -1 * bitCount(board.getPieceBB(BLACK_KNIGHT));
    score += material_values[BISHOP] * -1 * bitCount(board.getPieceBB(BLACK_BISHOP));
    score += material_values[ROOK] * -1 * bitCount(board.getPieceBB(BLACK_ROOK));
    score += material_values[QUEEN] * -1 * bitCount(board.getPieceBB(BLACK_QUEEN));

    return score;
}