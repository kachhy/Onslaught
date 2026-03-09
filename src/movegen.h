#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"
#include <vector>

using MoveList = std::vector<Move>;

enum MoveFlag {
  QUIET_MOVE_MOVEGEN = 0b01,
  NOISY_MOVE_MOVEGEN = 0b10,
  LEGAL_MOVE_MOVEGEN = QUIET_MOVE_MOVEGEN | NOISY_MOVE_MOVEGEN,
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
MoveList getLegalMoves(const Board &board);

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
        case BLACK_PAWN:
            return getPawnAttacks(square, BLACK);
        case WHITE_PAWN:
            return getPawnAttacks(square, WHITE);
        default:
            return BitBoard(0);
    }
}

constexpr inline void getMovesPawn(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_pawn_bb = board.getPieceBB(piece);
    Side cur_side = getPieceSide(piece);
    if (move_flag & QUIET_MOVE_MOVEGEN) {
        BitBoard empty_squares = ~board.getOcc(BOTH);
        BitBoard single_pushes = shiftPawnPushes(cur_pawn_bb, cur_side) & empty_squares;
        BitBoard double_pushes = shiftPawnPushes(single_pushes, cur_side) & getDoublePushRank(cur_side) & empty_squares;
        while (single_pushes > 0) {
            Square cur_to_square = static_cast<Square>(popLSB(double_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 8) : static_cast<Square>(cur_to_square - 8);
            if (single_pushes & (RANK_1 | RANK_8)) {
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, KNIGHT_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, BISHOP_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, ROOK_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUEEN_PROMO_FLAG | QUIET_FLAG));
            } else {
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUIET_FLAG));
            }
        }
        while (double_pushes > 0) {
            Square cur_to_square = static_cast<Square>(popLSB(double_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 16) : static_cast<Square>(cur_to_square - 16);
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUIET_FLAG));
        }
    }
    if (move_flag & NOISY_MOVE_MOVEGEN) {
        while (cur_pawn_bb > 0) {
            Square cur_from_square =  static_cast<Square>(popLSB(cur_pawn_bb));
            Square ep_square = board.getEPSquare();
            BitBoard cur_pawn_attacks = getPieceAttacks(board, piece, cur_from_square) & board.getOcc(getPieceSide(piece) == WHITE ? BLACK : WHITE) | BitBoard(ep_square);
            while(cur_pawn_attacks > 0) {
                Square cur_to_square = static_cast<Square>(popLSB(cur_pawn_attacks));
                if (cur_pawn_attacks & (RANK_1 | RANK_8)) {
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, KNIGHT_PROMO_FLAG | CAPTURE_FLAG));
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, BISHOP_PROMO_FLAG | CAPTURE_FLAG));
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, ROOK_PROMO_FLAG | CAPTURE_FLAG));
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUEEN_PROMO_FLAG | CAPTURE_FLAG));
                } else {
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, CAPTURE_FLAG));
                }
            }
        }
    }
}

constexpr inline void getMovesPiece(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_piece_bb = board.getPieceBB(piece);
    while (cur_piece_bb > 0) {
        Square cur_from_square = static_cast<Square>(popLSB(cur_piece_bb));
        if (move_flag & QUIET_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(board, piece, cur_from_square) & ~board.getOcc(getPieceSide(piece));
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, QUIET_FLAG));
            }
        }
        if (move_flag & NOISY_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(board, piece, cur_from_square) & board.getOcc(getPieceSide(piece));
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, CAPTURE_FLAG));
            }
        }
    }
}


#endif // MOVEGEN_H