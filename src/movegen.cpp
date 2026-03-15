#include "movegen.h"
#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"

MoveList getQuietMoves(const Board& board) {
    MoveList output;
    addLegalPawnMoves(output, board, makePiece(PAWN, board.getSTM()), QUIET_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        addPieceLegalMoves(output, board, static_cast<Piece>(i), QUIET_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getNoisyMoves(const Board& board) {
    MoveList output;
    addLegalPawnMoves(output, board, makePiece(PAWN, board.getSTM()), NOISY_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        addPieceLegalMoves(output, board, static_cast<Piece>(i), NOISY_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getLegalPieceMoves(const Board &board) {
    MoveList output;
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()) - 1; i++) {
        addPieceLegalMoves(output, board, static_cast<Piece>(i), LEGAL_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getLegalMoves(const Board& board) {
    MoveList out;
    if (bitCount(board.getCheckersMask()) > 1) {
        addLegalKingMoves(out, board);
        return out;
    }
    out = getLegalPieceMoves(board);
    addLegalKingMoves(out, board);
    addLegalPawnMoves(out, board, makePiece(PAWN, board.getSTM()), LEGAL_MOVE_MOVEGEN);
    addEPLegalMoves(out, board);
    return out;
}

void addLegalPawnMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard cur_pawn_bb = board.getPieceBB(piece);
    Side cur_side = getPieceSide(piece);
    BitBoard legal_mask = board.getLegalMask();
    Square king_sq = board.getKingSquare();
    if (move_flag & QUIET_MOVE_MOVEGEN) {
        BitBoard empty_squares = ~board.getOcc(BOTH);
        BitBoard single_pushes = shiftPawnPushes(cur_pawn_bb, cur_side) & empty_squares;
        BitBoard double_pushes = shiftPawnPushes(single_pushes, cur_side) & getDoublePushRank(cur_side) & empty_squares & legal_mask;
        single_pushes &= legal_mask;
        while (single_pushes > 0) {
            Square cur_to_square = static_cast<Square>(popLSB(single_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 8) : static_cast<Square>(cur_to_square - 8);
            BitBoard pin_mask = 0;
            if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
                pin_mask |= ray_squares[king_sq][cur_from_square];
            } else {
                pin_mask = ~BitBoard(0);
            }
            BitBoard cur_bb = (BitBoard(1) << cur_to_square) & pin_mask;
            if (cur_bb & (RANK_1 | RANK_8)) {
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, KNIGHT_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, BISHOP_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, ROOK_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUEEN_PROMO_FLAG | QUIET_FLAG));
            } else if (cur_bb > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUIET_FLAG));
            }
        }
        while (double_pushes > 0) {
            Square cur_to_square = static_cast<Square>(popLSB(double_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 16) : static_cast<Square>(cur_to_square - 16);
            BitBoard pin_mask = 0;
            if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
                pin_mask |= ray_squares[king_sq][cur_from_square];
            } else {
                pin_mask = ~BitBoard(0);
            }
            if (((BitBoard(1) << cur_to_square) & pin_mask) == 0) {
                continue;
            }
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, piece, QUIET_FLAG));
        }
    }
    if (move_flag & NOISY_MOVE_MOVEGEN) {
        while (cur_pawn_bb > 0) {
            Square cur_from_square = static_cast<Square>(popLSB(cur_pawn_bb));
            BitBoard pin_mask = 0;
            if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
                pin_mask |= ray_squares[king_sq][cur_from_square];
            } else {
                pin_mask = ~BitBoard(0);
            }
            BitBoard cur_pawn_attacks = (getPieceAttacks(piece, cur_from_square, board.getOcc(BOTH)) & (board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1)))) & legal_mask & pin_mask;
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

void addEPLegalMoves(MoveList& moves, const Board& board) {
    Square ep_square = board.getEPSquare();
    BitBoard ep_square_bb = BitBoard(1) << ep_square;
    if (ep_square == NO_SQUARE || !(board.getLegalMask() & ep_square_bb)) {
        return;
    }
    Side stm = board.getSTM();
    BitBoard movers = shiftPawnAttacks(ep_square_bb, board.getXSTM());
    while (movers > 0) {
        Square cur_from_square = static_cast<Square>(popLSB(movers));
        BitBoard pin_mask = 0;
        if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
            pin_mask |= ray_squares[board.getKingSquare()][cur_from_square];
        } else {
            pin_mask = ~BitBoard(0);
        }
        if ((pin_mask & ep_square_bb) > 0) {
            moves.emplace_back(GenerateMove(cur_from_square, ep_square, makePiece(PAWN, stm), EP_FLAG));
        }
    }
}

void addPieceLegalMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard legal_mask = board.getLegalMask();
    Square king_sq = board.getKingSquare();
    
    BitBoard cur_piece_bb = board.getPieceBB(piece);
    BitBoard occ_both = board.getOcc(BOTH);
    while (cur_piece_bb > 0) {
        Square cur_from_square = static_cast<Square>(popLSB(cur_piece_bb));
        BitBoard pin_mask = 0;
        if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
            pin_mask |= ray_squares[king_sq][cur_from_square];
        } else {
            pin_mask = ~BitBoard(0);
        }
        if (move_flag & QUIET_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(piece, cur_from_square, occ_both) & ~occ_both & legal_mask & pin_mask;
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, QUIET_FLAG));
            }
        }
        if (move_flag & NOISY_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(piece, cur_from_square, occ_both) & board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1)) & legal_mask & pin_mask;
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, CAPTURE_FLAG));
            }
        }
    }
}

void addLegalKingMoves(MoveList& moves, const Board& board) {
    Side stm = board.getSTM();
    Side xstm = board.getXSTM();
    Piece cur_piece = makePiece(KING, board.getSTM());
    Square cur_from_square = static_cast<Square>(getLSB(board.getPieceBB(cur_piece)));
    BitBoard king_moves = getKingAttacks(cur_from_square) & ~board.getThreatenedByXSTM() & ~board.getOcc(stm);
    BitBoard quiet_king_moves = king_moves & ~board.getOcc(xstm);
    king_moves &= board.getOcc(xstm);
    while (king_moves > 0) {
        Square cur_to_square = static_cast<Square>(popLSB(king_moves));
        moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, CAPTURE_FLAG));
    }
    while (quiet_king_moves > 0) {
        Square cur_to_square = static_cast<Square>(popLSB(quiet_king_moves));
        moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUIET_FLAG));
    }
    if (board.getCheckersMask() > 0) {
        return;
    }
    
    // 1111 = KQkq
    CastlingRights castling_rights = board.getCastlingRights();
    BitBoard occ = board.getOcc(BOTH);
    BitBoard white_threat_occ = board.getThreatenedBy(WHITE) | occ;
    BitBoard black_threat_occ = board.getThreatenedBy(BLACK) | occ;
    if (castling_rights & WHITE_KS && !(black_threat_occ & WHITE_KINGSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E1, G1, WHITE_KING, CASTLE_FLAG));
    }
    if (castling_rights & WHITE_QS && !(black_threat_occ & WHITE_QUEENSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E1, C1, WHITE_KING, CASTLE_FLAG));
    }
    if (castling_rights & BLACK_KS && !(white_threat_occ & BLACK_KINGSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E8, G8, BLACK_KING, CASTLE_FLAG));
    }
    if (castling_rights & BLACK_QS && !(white_threat_occ & BLACK_QUEENSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E8, C8, BLACK_KING, CASTLE_FLAG));
    }
}
