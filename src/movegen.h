#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"
#include <vector>

using MoveList = std::vector<Move>;

enum MoveFlag {
  QUIET_MOVES = 0b01,
  NOISY_MOVES = 0b10,
  LEGAL_MOVES = 0b11,
};

constexpr inline BitBoard getPromoRank(Side side) {
    return side == WHITE ? RANK_7 : RANK_2;
}

constexpr inline BitBoard getDoublePushRank(Side side) {
    return side == WHITE ? RANK_3 : RANK_6;
}

MoveList getQuietMoves(const Board& board);
MoveList getNoisyMoves(const Board& board);
MoveList getPseudoLegalMoves(const Board &board);

constexpr inline BitBoard getPieceAttacks(const Board& board, Piece piece, Square square) {
    switch (piece) {
        case BLACK_KING:
        case WHITE_KING:
            return getKingAttacks(square);
        case BLACK_QUEEN:
            return getQueenAttacks(square, board.getOcc(BLACK));
        case WHITE_QUEEN:
            return getQueenAttacks(square, board.getOcc(WHITE));
        case BLACK_ROOK:
            return getRookAttacks(square, board.getOcc(BLACK));
        case WHITE_ROOK:
            return getRookAttacks(square, board.getOcc(WHITE));
        case BLACK_BISHOP:
            return getBishopAttacks(square, board.getOcc(BLACK));
        case WHITE_BISHOP:
            return getBishopAttacks(square, board.getOcc(WHITE));
        case BLACK_KNIGHT:
        case WHITE_KNIGHT:
            return getKnightAttacks(square);
        default:
            return 0;
    }
}

constexpr inline void getMovesPawn(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_pawn_bb = board.getPieceBB(piece);
    Side cur_side = getPieceSide(piece);
    BitBoard empty_squares = ~board.getOcc(cur_side);
    BitBoard single_pushes = shiftPawnPushes(cur_pawn_bb, cur_side) & empty_squares;
    BitBoard double_pushes = shiftPawnPushes(single_pushes, cur_side) & getDoublePushRank(cur_side) & empty_squares;
    while (single_pushes > 0) {
         
    }
}

constexpr inline void getMovesPiece(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_piece_bb = board.getPieceBB(piece);
    while (cur_piece_bb > 0) {
        Square piece_square = static_cast<Square>(popLSB(cur_piece_bb));
        if (move_flag & QUIET_MOVES) {
            BitBoard piece_attacks = getPieceAttacks(board, piece, piece_square) & ~board.getOcc(getPieceSide(piece));
            while (piece_attacks > 0) {
                moves.emplace_back(GenerateMove(piece_square, static_cast<Square>(popLSB(piece_attacks)), piece, QUIET_FLAG));
            }
        }
        if (move_flag & NOISY_MOVES) {
            BitBoard piece_attacks = getPieceAttacks(board, piece, piece_square) & board.getOcc(getPieceSide(piece));
            while (piece_attacks > 0) {
                moves.emplace_back(GenerateMove(piece_square, static_cast<Square>(popLSB(piece_attacks)), piece, CAPTURE_FLAG));
            }
        }
    }
}


#endif // MOVEGEN_H