#include "movegen.h"
#include "attacks.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"

MoveList getQuietMoves(const Board& board) {
    MoveList output;
    addPseudoLegalMovesPawn(output, board, makePiece(PAWN, board.getSTM()), QUIET_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        addPieceLegalMoves(output, board, static_cast<Piece>(i), QUIET_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getNoisyMoves(const Board& board) {
    MoveList output;
    addPseudoLegalMovesPawn(output, board, makePiece(PAWN, board.getSTM()), NOISY_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()); i++) {
        addPieceLegalMoves(output, board, static_cast<Piece>(i), NOISY_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getPseudoLegalMoves(const Board &board) {
    MoveList output;
    addPseudoLegalMovesPawn(output, board, makePiece(PAWN, board.getSTM()), LEGAL_MOVE_MOVEGEN);
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()) - 1; i++) {
        addPieceLegalMoves(output, board, static_cast<Piece>(i), LEGAL_MOVE_MOVEGEN);
    }
    return output;
}

MoveList getLegalMoves(const Board& board) {
    MoveList out;
    int num_checkers = bitCount(board.getCheckersMask());
    if (num_checkers > 1) {
        addLegalKingMoves(out, board);
        return out;
    }
    out = getPseudoLegalMoves(board);
    addLegalKingMoves(out, board);
    return out;
}

void addPseudoLegalMovesPawn(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
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
            Square cur_from_square = static_cast<Square>(popLSB(cur_pawn_bb));
            Square ep_square = board.getEPSquare();
            BitBoard cur_pawn_attacks = getPieceAttacks(piece, cur_from_square, board.getOcc(BOTH)) & (board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1)) | (ep_square == NO_SQUARE ? 0 : BitBoard(1) << ep_square));
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

void addPieceLegalMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard checkers = board.getCheckersMask();
    BitBoard legal_mask = 0;

    if (checkers == 0) {
        legal_mask = ~BitBoard(0);
    } else { // checkers == 1
        Square king_sq = board.getKingSquare();
        Square checker_sq = static_cast<Square>(getLSB(checkers));
        legal_mask = checkers;
        Piece checker_piece = board.pieceAt(static_cast<uint8_t>(checker_sq));
        DefaultPiece checker_type = makeDefaultPiece(checker_piece);
        if (checker_type == BISHOP || checker_type == ROOK || checker_type == QUEEN) {
            legal_mask |= between_squares[king_sq][checker_sq];
        }
    }
    BitBoard cur_piece_bb = board.getPieceBB(piece);
    BitBoard occ_both = board.getOcc(BOTH);
    while (cur_piece_bb > 0) {
        Square cur_from_square = static_cast<Square>(popLSB(cur_piece_bb));
        if (move_flag & QUIET_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(piece, cur_from_square, occ_both) & ~occ_both & legal_mask;
            while (cur_piece_attacks > 0) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(cur_piece_attacks)), piece, QUIET_FLAG));
            }
        }
        if (move_flag & NOISY_MOVE_MOVEGEN) {
            BitBoard cur_piece_attacks = getPieceAttacks(piece, cur_from_square, occ_both) & board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1)) & legal_mask;
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
    if (castling_rights & 0b1000 && !(black_threat_occ & WHITE_KINGSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E1, G1, WHITE_KING, CASTLE_FLAG));
    }
    if (castling_rights & 0b0100 && !(black_threat_occ & WHITE_QUEENSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E1, C1, WHITE_KING, CASTLE_FLAG));
    }
    if (castling_rights & 0b0010 && !(white_threat_occ & BLACK_KINGSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E8, G8, BLACK_KING, CASTLE_FLAG));
    }
    if (castling_rights & 0b0001 && !(white_threat_occ & BLACK_QUEENSIDE_CASTLE_MASK)) {
        moves.emplace_back(GenerateMove(E8, C8, BLACK_KING, CASTLE_FLAG));
    }
}
