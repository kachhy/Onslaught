#include "movepicker.h"
#include "movegen/attacks.h"
#include "see.h"
#include <utility>

// Will also return false if the move is NO_MOVE
static bool isLegal(Move move, const Board& board) {
    if (move == NO_MOVE) {
        return false;
    }

    Square from = From(move);
    Square to = To(move);
    Piece piece = MovePiece(move);
    DefaultPiece pt = makeDefaultPiece(piece);
    Side stm = board.getSTM();
    Side xstm = board.getXSTM();

    if (getPieceSide(piece) != stm || piece != board.pieceAt(from)) {
        return false;
    }

    if (Castle(move)) {
        if (board.getCheckersMask() || !board.getCastlingRights()) {
            return false;
        }

        CastlingRights cr = board.getCastlingRights();
        BitBoard occ = board.getOcc(BOTH);
        BitBoard xstm_threats = board.getThreatenedByXSTM();

        if (stm == WHITE) {
            if (to == G1) {
                return (cr & WHITE_KS) && !((xstm_threats | occ) & WHITE_KINGSIDE_CASTLE_MASK);
            }

            if (to == C1) {
                return (cr & WHITE_QS) && !((xstm_threats & WHITE_QUEENSIDE_CASTLE_THREAT_MASK) | (occ & WHITE_QUEENSIDE_CASTLE_OCC_MASK));
            }
            return false;
        } else {
            if (to == G8) {
                return (cr & BLACK_KS) && !((xstm_threats | occ) & BLACK_KINGSIDE_CASTLE_MASK);
            }

            if (to == C8) {
                return (cr & BLACK_QS) && !((xstm_threats & BLACK_QUEENSIDE_CASTLE_THREAT_MASK) | (occ & BLACK_QUEENSIDE_CASTLE_OCC_MASK));
            }

            return false;
        }
    }

    // A pinned piece may only move along the ray between the king and its pinner.
    BitBoard pin_mask = ~BitBoard(0);
    if (pt != KING && getBit(board.getPinMask(), from)) {
        pin_mask = line_squares[board.getKingSquare()][from];
    }

    if (Prom(move)) {
        BitBoard checkers = board.getCheckersMask();
        if (multipleActiveBits(checkers)) {
            return false;
        }

        Square king = board.getKingSquare();
        BitBoard valid = checkers ? (checkers | between_squares[king][getLSB(checkers)]) : ~BitBoard(0);
        BitBoard goal = stm == WHITE ? RANK_8 : RANK_1;
        BitBoard opts = (shiftPawnPushes(BitBoard(1) << from, stm) & ~board.getOcc(BOTH)) | (getPawnAttacks(from, stm) & board.getOcc(xstm));

        return static_cast<bool>(getBit(goal & opts & valid & pin_mask, to));
    }

    if (Capture(move) && !IsEP(move) && board.pieceAt(to) == NO_PIECE) {
        return false;
    }

    if (!Capture(move) && board.pieceAt(to) != NO_PIECE) {
        return false;
    }

    if (IsEP(move) && to != board.getEPSquare()) {
        return false;
    }

    if (getBit(board.getOcc(stm), to)) {
        return false;
    }

    if (pt == KING) {
        if (getBit(board.getThreatenedByXSTM(), to)) {
            return false;
        }

        // The king may not step to a square still covered by a sliding checker along the ray behind it (as threatened_by rays stops at the king)
        BitBoard checkers = board.getCheckersMask();
        while (checkers) {
            Square checker_sq = static_cast<Square>(popLSB(checkers));
            DefaultPiece checker_pt = makeDefaultPiece(board.pieceAt(checker_sq));

            if (checker_pt == PAWN || checker_pt == KNIGHT) {
                continue;
            }

            BitBoard ray = line_squares[from][checker_sq];
            popBit(ray, checker_sq);

            if (getBit(ray, to)) {
                return false;
            }
        }
    }

    if (pt == PAWN) {
        if (getBit(RANK_1 | RANK_8, to)) {
            return false;
        }

        if (!IsEP(move)) {
            bool is_capture = getBit(getPawnAttacks(from, stm) & board.getOcc(xstm), to);
            bool is_single_push = (stm == WHITE ? from - 8 == to : from + 8 == to) && board.pieceAt(to) == NO_PIECE;
            bool on_start_rank = stm == WHITE ? getRank(from) == 6 : getRank(from) == 1;
            bool is_double_push = (stm == WHITE ? from - 16 == to : from + 16 == to) && board.pieceAt(to) == NO_PIECE &&
                                  board.pieceAt(stm == WHITE ? to + 8 : to - 8) == NO_PIECE && on_start_rank;

            if (!is_capture && !is_single_push && !is_double_push) {
                return false;
            }
        }
    } else if (!getBit(getPieceAttacks(piece, from, board.getOcc(BOTH)), to)) {
        return false;
    }

    if (!getBit(pin_mask, to)) {
        return false;
    }

    if (board.getCheckersMask() && pt != KING) {
        if (multipleActiveBits(board.getCheckersMask())) {
            return false;
        }

        Square king_sq = board.getKingSquare();
        BitBoard blocks_and_captures = board.getCheckersMask() | between_squares[king_sq][getLSB(board.getCheckersMask())];
        Square to2 = IsEP(move) ? static_cast<Square>(stm == WHITE ? to + 8 : to - 8) : to;
        
        if (!getBit(blocks_and_captures, to2)) {
            return false;
        }
    }

    return true;
}

int MovePicker::scoreQuietMove(Move move) {
    return getScoreHistory(board.getSTM(), move) + getContHist(ss, board.getSTM(), move);
}

int MovePicker::scoreLoudMove(Move move) {
    // Good SEE capture
    if ((Capture(move) || IsEP(move)) && staticExchangeEval(board, move, 0)) {
        DefaultPiece attacker = makeDefaultPiece(MovePiece(move));
        Piece victim_piece = IsEP(move) ? makePiece(PAWN, board.getXSTM()) : board.pieceAt(To(move));
        DefaultPiece victim = makeDefaultPiece(victim_piece);
        return 50000 + MVV_LVA[victim][attacker];
    }

    if (Prom(move)) {
        return 40000;
    }

    // Bad SEE capture
    DefaultPiece attacker = makeDefaultPiece(MovePiece(move));
    Piece victim_piece = IsEP(move) ? makePiece(PAWN, board.getXSTM()) : board.pieceAt(To(move));
    DefaultPiece victim = makeDefaultPiece(victim_piece);
    return MVV_LVA[victim][attacker];
}

void MovePicker::initCaptures() {
    getNoisyMoves(board, loud_moves);
    for (uint16_t i = 0; i < loud_moves.size(); i++) {
        loud_scores[i] = scoreLoudMove(loud_moves[i]);
    }
}

void MovePicker::initQuiets() {
    getQuietMoves(board, quiet_moves);
    for (uint16_t i = 0; i < quiet_moves.size(); i++) {
        quiet_scores[i] = scoreQuietMove(quiet_moves[i]);
    }
}

Move MovePicker::nextMove() {
    switch (stage) {
    case TT_MOVE:
        stage = INIT_CAPTURES;
        if (isLegal(tt_move, board) && (type == REGULAR || Capture(tt_move) || IsEP(tt_move) || Prom(tt_move))) {
            return tt_move;
        }
    case INIT_CAPTURES: stage = GOOD_CAPTURES; initCaptures();
    case GOOD_CAPTURES:
        while (loud_moves_start < loud_moves.size()) {
            int best_index = loud_moves_start;

            for (uint16_t j = loud_moves_start + 1; j < loud_moves.size(); j++) {
                if (loud_scores[j] > loud_scores[best_index]) {
                    best_index = j;
                }
            }

            if (loud_scores[best_index] < 40000) { // Only bad captures remain
                break;
            }

            Move best_move = loud_moves[best_index];
            loud_moves.sort_item(best_index);
            std::swap(loud_scores[loud_moves_start], loud_scores[best_index]);
            loud_moves_start++;

            if (best_move == tt_move) {
                continue; // already yielded at the TT stage
            }

            return best_move;
        }

        if (type == QSEARCH) {
            stage = BAD_CAPTURES;
        } else {
            stage = KILLER1;
        }
    case KILLER1:
        stage = KILLER2;
        if (ss->killers[0] != tt_move && isLegal(ss->killers[0], board)) {
            return ss->killers[0];
        }
    case KILLER2:
        stage = BAD_CAPTURES;
        if (ss->killers[1] != tt_move && isLegal(ss->killers[1], board)) {
            return ss->killers[1];
        }
    case BAD_CAPTURES:
        while (loud_moves_start < loud_moves.size()) {
            int best_index = loud_moves_start;

            for (uint16_t j = loud_moves_start + 1; j < loud_moves.size(); j++) {
                if (loud_scores[j] > loud_scores[best_index]) {
                    best_index = j;
                }
            }

            Move best_move = loud_moves[best_index];
            loud_moves.sort_item(best_index);
            std::swap(loud_scores[loud_moves_start], loud_scores[best_index]);
            loud_moves_start++;

            if (best_move == tt_move) {
                continue; // already yielded at the TT stage
            }

            return best_move;
        }

        if (type == QSEARCH) {
            stage = END;
        } else {
            stage = INIT_QUIET;
        }
    case INIT_QUIET: stage = QUIET_MOVES; initQuiets();
    case QUIET_MOVES:
        while (quiet_moves_start < quiet_moves.size()) {
            int best_index = quiet_moves_start;

            for (uint16_t j = quiet_moves_start + 1; j < quiet_moves.size(); j++) {
                if (quiet_scores[j] > quiet_scores[best_index]) {
                    best_index = j;
                }
            }

            Move best_move = quiet_moves[best_index];
            quiet_moves.sort_item(best_index);
            std::swap(quiet_scores[quiet_moves_start], quiet_scores[best_index]);
            quiet_moves_start++;

            if (best_move == tt_move || best_move == ss->killers[0] || best_move == ss->killers[1]) {
                continue; // already yielded at the TT/killer stages
            }

            return best_move;
        }

        stage = END;
    default:
        case END:
            return NO_MOVE;
    }
}