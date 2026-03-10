#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"

MoveList getQuietMoves(const Board& board) {
    MoveList output;
    getMovesPawn(output, board, makePiece(PAWN, board.getSTM()), QUIET_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        getMovesPiece(output, board, static_cast<Piece>(i), QUIET_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getNoisyMoves(const Board& board) {
    MoveList output;
    getMovesPawn(output, board, makePiece(PAWN, board.getSTM()), NOISY_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        getMovesPiece(output, board, static_cast<Piece>(i), NOISY_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getPseudoLegalMoves(const Board &board) {
    MoveList output;
    if (bitCount(board.getCheckersMask()) > 1) {
        
    }
    getMovesPawn(output, board, makePiece(PAWN, board.getSTM()), LEGAL_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        getMovesPiece(output, board, static_cast<Piece>(i), LEGAL_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getLegalMoves(const Board& board) {
    MoveList out;
    out = getPseudoLegalMoves(board);
    return out;
}

BitBoard getPieceAttacks(const Board& board, Piece piece, Square square) {
    switch (piece) {
        case BLACK_KING:
        case WHITE_KING:
            return getKingAttacks(square);
        case BLACK_QUEEN:
        case WHITE_QUEEN:
            return getQueenAttacks(square, board.getOcc(BOTH));
        case BLACK_ROOK:
        case WHITE_ROOK:
            return getRookAttacks(square, board.getOcc(BOTH));
        case BLACK_BISHOP:
        case WHITE_BISHOP:
            return getBishopAttacks(square, board.getOcc(BOTH));
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

void getMovesPawn(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_pawn_bb = board.getPieceBB(piece);
    Side cur_side = getPieceSide(piece);
    if (move_flag & QUIET_MOVE_MOVEGEN) {
        BitBoard empty_squares = ~board.getOcc(BOTH);
        BitBoard single_pushes = shiftPawnPushes(cur_pawn_bb, cur_side) & empty_squares;
        BitBoard double_pushes = shiftPawnPushes(single_pushes, cur_side) & getDoublePushRank(cur_side) & empty_squares;
        while (single_pushes > 0) {
            Square cur_to_square = static_cast<Square>(popLSB(single_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 8) : static_cast<Square>(cur_to_square - 8);
            if ((BitBoard(1) << cur_to_square) & (RANK_1 | RANK_8)) {
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
            BitBoard cur_pawn_attacks = getPieceAttacks(board, piece, cur_from_square) & (board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1)) | (ep_square == NO_SQUARE ? 0 : BitBoard(1) << ep_square));
            while(cur_pawn_attacks > 0) {
                Square cur_to_square = static_cast<Square>(popLSB(cur_pawn_attacks));
                if ((BitBoard(1) << cur_to_square) & (RANK_1 | RANK_8)) {
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

void getMovesPiece(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_piece_bb = board.getPieceBB(piece);
    while (cur_piece_bb > 0) {
        Square cur_from_square = static_cast<Square>(popLSB(cur_piece_bb));
        if (move_flag & QUIET_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(board, piece, cur_from_square) & ~board.getOcc(BOTH);
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, QUIET_FLAG));
            }
        }
        if (move_flag & NOISY_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(board, piece, cur_from_square) & board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1));
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, CAPTURE_FLAG));
            }
        }
    }
}
