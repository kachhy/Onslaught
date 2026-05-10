#include "movegen.h"
#include "attacks.h"
#include "core/movelist.h"

void getQuietMoves(const Board& board, MoveList& moves) {
    if (multipleActiveBits(board.getCheckersMask())) {
        addLegalKingMoves(moves, board, QUIET_MOVE_MOVEGEN);
        return;
    }
    addAllQuietPieceMoves(moves, board);
    addLegalKingMoves(moves, board, QUIET_MOVE_MOVEGEN);
    addLegalPawnMoves(moves, board, QUIET_MOVE_MOVEGEN);
}

void getNoisyMoves(const Board& board, MoveList& moves) {
    if (multipleActiveBits(board.getCheckersMask())) {
        addLegalKingMoves(moves, board, NOISY_MOVE_MOVEGEN);
        return;
    }
    addAllNoisyPieceMoves(moves, board);
    addLegalKingMoves(moves, board, NOISY_MOVE_MOVEGEN);
    addLegalPawnMoves(moves, board, NOISY_MOVE_MOVEGEN);
    addEPLegalMoves(moves, board);
}

MoveList getLegalCheckingMoves(const Board& board) {
    MoveList out;

    return out;
}

void getLegalMoves(const Board& board, MoveList& moves) {
    if (multipleActiveBits(board.getCheckersMask())) {
        addLegalKingMoves(moves, board, LEGAL_MOVE_MOVEGEN);
        return;
    }
    addAllLegalPieceMoves(moves, board);
    addLegalKingMoves(moves, board, LEGAL_MOVE_MOVEGEN);
    addLegalPawnMoves(moves, board, LEGAL_MOVE_MOVEGEN);
    addEPLegalMoves(moves, board);
}

void addLegalPawnMoves(MoveList& moves, const Board& board, MoveFlag move_flag) {
    Side cur_side = board.getSTM();
    Piece cur_piece = makePiece(PAWN, cur_side);
    BitBoard cur_pawn_bb = board.getPieceBB(cur_piece);
    BitBoard legal_mask = board.getLegalMask();
    Square king_sq = board.getKingSquare();
    if (move_flag & QUIET_MOVE_MOVEGEN) {
        BitBoard empty_squares = ~board.getOcc(BOTH);
        BitBoard single_pushes = shiftPawnPushes(cur_pawn_bb, cur_side) & empty_squares;
        BitBoard double_pushes = shiftPawnPushes(single_pushes, cur_side) & getDoublePushRank(cur_side) & empty_squares & legal_mask;
        single_pushes &= legal_mask;
        while (single_pushes) {
            Square cur_to_square = static_cast<Square>(popLSB(single_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 8) : static_cast<Square>(cur_to_square - 8);
            BitBoard pin_mask = 0;
            if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
                pin_mask |= line_squares[king_sq][cur_from_square];
            } else {
                pin_mask = ~BitBoard(0);
            }
            BitBoard cur_bb = (BitBoard(1) << cur_to_square) & pin_mask;
            if (cur_bb & (RANK_1 | RANK_8)) {
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, KNIGHT_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, BISHOP_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, ROOK_PROMO_FLAG | QUIET_FLAG));
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUEEN_PROMO_FLAG | QUIET_FLAG));
            } else if (cur_bb) {
                moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUIET_FLAG));
            }
        }
        while (double_pushes) {
            Square cur_to_square = static_cast<Square>(popLSB(double_pushes));
            Square cur_from_square = cur_side == WHITE ? static_cast<Square>(cur_to_square + 16) : static_cast<Square>(cur_to_square - 16);
            BitBoard pin_mask = 0;
            if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
                pin_mask |= line_squares[king_sq][cur_from_square];
            } else {
                pin_mask = ~BitBoard(0);
            }
            if (((BitBoard(1) << cur_to_square) & pin_mask) == 0) {
                continue;
            }
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUIET_FLAG));
        }
    }
    if (move_flag & NOISY_MOVE_MOVEGEN) {
        BitBoard pin_bb = board.getPinMask();
        BitBoard pinned_pawns = cur_pawn_bb & pin_bb;
        BitBoard unpinned_pawns = cur_pawn_bb & ~pin_bb;
        BitBoard enemy = board.getOcc(static_cast<Side>(cur_side ^ 1));
        constexpr BitBoard PROMO_RANK = RANK_1 | RANK_8;

        BitBoard west_caps = shiftPawnCapturesWest(unpinned_pawns, cur_side) & enemy & legal_mask;
        BitBoard east_caps = shiftPawnCapturesEast(unpinned_pawns, cur_side) & enemy & legal_mask;

        const int8_t west_delta = (cur_side == WHITE) ? 9 : -7;
        const int8_t east_delta = (cur_side == WHITE) ? 7 : -9;

        BitBoard west_promo = west_caps & PROMO_RANK;
        BitBoard west_normal = west_caps & ~PROMO_RANK;
        while (west_normal) {
            Square cur_to_square = static_cast<Square>(popLSB(west_normal));
            Square cur_from_square = static_cast<Square>(cur_to_square + west_delta);
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, CAPTURE_FLAG));
        }
        while (west_promo) {
            Square cur_to_square = static_cast<Square>(popLSB(west_promo));
            Square cur_from_square = static_cast<Square>(cur_to_square + west_delta);
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, KNIGHT_PROMO_FLAG | CAPTURE_FLAG));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, BISHOP_PROMO_FLAG | CAPTURE_FLAG));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, ROOK_PROMO_FLAG | CAPTURE_FLAG));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUEEN_PROMO_FLAG | CAPTURE_FLAG));
        }

        BitBoard east_promo = east_caps & PROMO_RANK;
        BitBoard east_normal = east_caps & ~PROMO_RANK;
        while (east_normal) {
            Square cur_to_square = static_cast<Square>(popLSB(east_normal));
            Square cur_from_square = static_cast<Square>(cur_to_square + east_delta);
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, CAPTURE_FLAG));
        }
        while (east_promo) {
            Square cur_to_square = static_cast<Square>(popLSB(east_promo));
            Square cur_from_square = static_cast<Square>(cur_to_square + east_delta);
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, KNIGHT_PROMO_FLAG | CAPTURE_FLAG));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, BISHOP_PROMO_FLAG | CAPTURE_FLAG));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, ROOK_PROMO_FLAG | CAPTURE_FLAG));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUEEN_PROMO_FLAG | CAPTURE_FLAG));
        }

        while (pinned_pawns) {
            Square cur_from_square = static_cast<Square>(popLSB(pinned_pawns));
            BitBoard pin_mask = line_squares[king_sq][cur_from_square];
            BitBoard cur_pawn_attacks = getPawnAttacks(cur_from_square, cur_side) & enemy & legal_mask & pin_mask;
            while (cur_pawn_attacks) {
                Square cur_to_square = static_cast<Square>(popLSB(cur_pawn_attacks));
                if ((BitBoard(1) << cur_to_square) & PROMO_RANK) {
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, KNIGHT_PROMO_FLAG | CAPTURE_FLAG));
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, BISHOP_PROMO_FLAG | CAPTURE_FLAG));
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, ROOK_PROMO_FLAG | CAPTURE_FLAG));
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUEEN_PROMO_FLAG | CAPTURE_FLAG));
                } else {
                    moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, CAPTURE_FLAG));
                }
            }
        }
    }
}

void addEPLegalMoves(MoveList& moves, const Board& board) {
    Square ep_square = board.getEPSquare();
    if (ep_square == NO_SQUARE) {
        return;
    }
    BitBoard ep_square_bb = BitBoard(1) << ep_square;
    Side xstm = board.getXSTM();
    BitBoard shifted_ep_square = shiftPawnPushes(ep_square_bb, xstm);
    if (!(board.getLegalMask() & shifted_ep_square)) {
        return;
    }
    Side stm = board.getSTM();
    BitBoard movers = shiftPawnAttacks(ep_square_bb, xstm) & board.getPieceBB(makePiece(PAWN, stm));
    while (movers) {
        Square cur_from_square = static_cast<Square>(popLSB(movers));
        BitBoard pin_mask = 0;
        if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
            pin_mask |= line_squares[board.getKingSquare()][cur_from_square];
        } else {
            pin_mask = ~BitBoard(0);
        }
        if (!(pin_mask & ep_square_bb)) {
            continue;
        }
        BitBoard ep_rank = getAttackingEPRank(stm);
        BitBoard enemy_sliders_on_ep_rank = (ep_rank & (board.getPieceBB(makePiece(ROOK, xstm)) | board.getPieceBB(makePiece(QUEEN, xstm))));
        BitBoard king_on_ep_rank = (ep_rank & board.getPieceBB(makePiece(KING, stm)));
        if (enemy_sliders_on_ep_rank && king_on_ep_rank) {
            BitBoard occ_no_ep_pawns = board.getOcc(BOTH) & ~(shifted_ep_square | (BitBoard(1) << cur_from_square));
            bool check_risk = false;
            while (enemy_sliders_on_ep_rank) {
                Square cur_slider_square = static_cast<Square>(popLSB(enemy_sliders_on_ep_rank));
                if (!(occ_no_ep_pawns & between_squares[cur_slider_square][getLSB(king_on_ep_rank)])) {
                    check_risk = true;
                    break;
                }
            }
            if (check_risk) {
                continue;
            }
        }
        moves.emplace_back(GenerateMove(cur_from_square, ep_square, makePiece(PAWN, stm), EP_FLAG));
    }
}

void addPieceLegalMoves(MoveList& moves, const Board& board, Piece piece, MoveFlag move_flag) {
    BitBoard legal_mask = board.getLegalMask();
    Square king_sq = board.getKingSquare();
    BitBoard cur_piece_bb = board.getPieceBB(piece);
    if (makeDefaultPiece(piece) == KNIGHT) {
        cur_piece_bb &= ~board.getPinMask();
    }
    BitBoard occ_both = board.getOcc(BOTH);
    while (cur_piece_bb) {
        Square cur_from_square = static_cast<Square>(popLSB(cur_piece_bb));
        BitBoard pin_mask = 0;
        if (board.getPinMask() & (BitBoard(1) << cur_from_square)) {
            pin_mask |= line_squares[king_sq][cur_from_square];
        } else {
            pin_mask = ~BitBoard(0);
        }
        BitBoard cur_piece_attacks = getPieceAttacks(piece, cur_from_square, occ_both) & legal_mask & pin_mask;
        if (move_flag & QUIET_MOVE_MOVEGEN) {
            BitBoard quiet_cur_piece_attacks = cur_piece_attacks & ~occ_both;
            while (quiet_cur_piece_attacks) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(quiet_cur_piece_attacks)), piece, QUIET_FLAG));
            }
        }
        if (move_flag & NOISY_MOVE_MOVEGEN) {
            BitBoard noisy_cur_piece_attacks = cur_piece_attacks & board.getOcc(static_cast<Side>(getPieceSide(piece) ^ 1));
            while (noisy_cur_piece_attacks) {
                moves.emplace_back(GenerateMove(cur_from_square, static_cast<Square>(popLSB(noisy_cur_piece_attacks)), piece, CAPTURE_FLAG));
            }
        }
    }
}

void addAllLegalPieceMoves(MoveList& moves, const Board& board) {
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()) - 1; i++) {
        addPieceLegalMoves(moves, board, static_cast<Piece>(i), LEGAL_MOVE_MOVEGEN);
    }
}

void addAllLegalCheckingPieceMoves(MoveList &moves, const Board &board) {
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()) - 1; i++) {
        addPieceLegalMoves(moves, board, static_cast<Piece>(i), CHECKING_MOVE_MOVEGEN);
    }
}

void addAllNoisyPieceMoves(MoveList& moves, const Board& board) {
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()) - 1; i++) {
        addPieceLegalMoves(moves, board, static_cast<Piece>(i), NOISY_MOVE_MOVEGEN);
    }
}

void addAllQuietPieceMoves(MoveList& moves, const Board& board) {
    for (int i = makePiece(PAWN, board.getSTM()) + 1; i <= makePiece(KING, board.getSTM()) - 1; i++) {
        addPieceLegalMoves(moves, board, static_cast<Piece>(i), QUIET_MOVE_MOVEGEN);
    }
}

void addLegalKingMoves(MoveList& moves, const Board& board, MoveFlag move_flag) {
    Side stm = board.getSTM();
    Side xstm = board.getXSTM();
    Piece cur_piece = makePiece(KING, stm);
    Square cur_from_square = static_cast<Square>(getLSB(board.getPieceBB(cur_piece)));
    BitBoard checkers = board.getCheckersMask();
    BitBoard checker_rays = 0;
    while (checkers) {
        int checker_square = popLSB(checkers);
        DefaultPiece checker_piece = makeDefaultPiece(board.pieceAt(checker_square));
        if (checker_piece == PAWN || checker_piece == KNIGHT) {
            continue;
        }
        BitBoard cur_line = line_squares[cur_from_square][checker_square];
        popBit(cur_line, checker_square);
        checker_rays |= cur_line;
    }
    BitBoard king_moves = getKingAttacks(cur_from_square) & ~board.getThreatenedByXSTM() & ~board.getOcc(stm) & ~checker_rays;
    if (move_flag & QUIET_MOVE_MOVEGEN) {
        BitBoard quiet_king_moves = king_moves & ~board.getOcc(xstm);
        while (quiet_king_moves) {
            Square cur_to_square = static_cast<Square>(popLSB(quiet_king_moves));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, QUIET_FLAG));
        }
    }
    if (move_flag & NOISY_MOVE_MOVEGEN) {
        king_moves &= board.getOcc(xstm);
        while (king_moves) {
            Square cur_to_square = static_cast<Square>(popLSB(king_moves));
            moves.emplace_back(GenerateMove(cur_from_square, cur_to_square, cur_piece, CAPTURE_FLAG));
        }
    }
    if (board.getCheckersMask() || (move_flag == NOISY_MOVE_MOVEGEN)) {
        return;
    }

    // 1111 = KQkq
    CastlingRights castling_rights = board.getCastlingRights();
    BitBoard occ = board.getOcc(BOTH);
    BitBoard xstm_threats = board.getThreatenedByXSTM();
    if (stm == WHITE) {
        if (castling_rights & WHITE_KS && !((xstm_threats | occ) & WHITE_KINGSIDE_CASTLE_MASK)) {
            moves.emplace_back(GenerateMove(E1, G1, WHITE_KING, CASTLE_FLAG));
        }
        if (castling_rights & WHITE_QS && !((xstm_threats & WHITE_QUEENSIDE_CASTLE_THREAT_MASK) | (occ & WHITE_QUEENSIDE_CASTLE_OCC_MASK))) {
            moves.emplace_back(GenerateMove(E1, C1, WHITE_KING, CASTLE_FLAG));
        }
    } else {
        if (castling_rights & BLACK_KS && !((xstm_threats | occ) & BLACK_KINGSIDE_CASTLE_MASK)) {
            moves.emplace_back(GenerateMove(E8, G8, BLACK_KING, CASTLE_FLAG));
        }
        if (castling_rights & BLACK_QS && !((xstm_threats & BLACK_QUEENSIDE_CASTLE_THREAT_MASK) | (occ & BLACK_QUEENSIDE_CASTLE_OCC_MASK))) {
            moves.emplace_back(GenerateMove(E8, C8, BLACK_KING, CASTLE_FLAG));
        }
    }
}
