#include "move.h"
#include "board/board.h"
#include "core/bitboard.h"
#include "core/types.h"
#include "movegen/attacks.h"
#include <cmath>

std::string moveToStr(Move move) {
    if (move == NO_MOVE) {
        return "0000";
    }

    auto squareToStr = [](Square sq) -> std::string {
        int s = static_cast<int>(sq);
        std::string result;
        result += static_cast<char>('a' + (s % 8));
        result += static_cast<char>('0' + (8 - (s / 8)));
        return result;
    };

    std::string result = squareToStr(From(move)) + squareToStr(To(move));

    if (Prom(move)) {
        constexpr char promoPieces[] = { 'p', 'n', 'b', 'r', 'q', 'k' }; // Promote to king should never happen but just in case
        result += promoPieces[static_cast<int>(promPiece(move))];
    }

    return result;
}

Move strToMove(const std::string& str, const Board& board) {
    if (str.length() < 4 || str == "0000") {
        return NO_MOVE;
    }

    int from_file = str[0] - 'a';
    int from_rank = '8' - str[1];
    int to_file = str[2] - 'a';
    int to_rank = '8' - str[3];

    if (from_file < 0 || from_file > 7 || from_rank < 0 || from_rank > 7 || to_file < 0 || to_file > 7 || to_rank < 0 || to_rank > 7) {
        return NO_MOVE;
    }

    Square from = static_cast<Square>(from_rank * 8 + from_file);
    Square to = static_cast<Square>(to_rank * 8 + to_file);

    Piece piece = board.pieceAt(from);
    if (piece == NO_PIECE) {
        return NO_MOVE;
    }

    uint32_t flags = QUIET_FLAG;
    Piece target = board.pieceAt(to);

    if (target != NO_PIECE) {
        flags = CAPTURE_FLAG;
    }

    DefaultPiece dp = makeDefaultPiece(piece);
    if (dp == PAWN) {
        // En passant
        if (from_file != to_file && target == NO_PIECE) {
            flags = EP_FLAG;
        }

        // Promotion
        if (str.length() == 5) {
            switch (str[4]) {
            case 'n': flags = KNIGHT_PROMO_FLAG; break;
            case 'b': flags = BISHOP_PROMO_FLAG; break;
            case 'r': flags = ROOK_PROMO_FLAG; break;
            case 'q': flags = QUEEN_PROMO_FLAG; break;
            default: break;
            }
            if (target != NO_PIECE) {
                flags |= CAPTURE_FLAG;
            }
        }
    } else if (dp == KING) {
        if (std::abs(from_file - to_file) == 2) {
            flags = CASTLE_FLAG;
        }
    }

    return GenerateMove(from, to, piece, flags);
}

DefaultPiece promPiece(Move move) {
    uint32_t flags = Flags(move);
    if ((flags & QUEEN_PROMO_FLAG) == QUEEN_PROMO_FLAG) {
        return QUEEN;
    }
    if ((flags & ROOK_PROMO_FLAG) == ROOK_PROMO_FLAG) {
        return ROOK;
    }
    if ((flags & BISHOP_PROMO_FLAG) == BISHOP_PROMO_FLAG) {
        return BISHOP;
    }
    return KNIGHT;
}

bool givesCheck(const Board& board, Move move) {
    // edge cases:
    //  X1: Ensure to get rid of From(move) from occ, otherwise slider sliding away from king will not give check.
    //  X2: Promo: promoting to a new piece will turn pawn to piece and can attack king
    //  X3: pins: moving can reveal slider checks
    //  X3.5: ep: en passant can get rid of 2 pawns on 1 file, possibly revealing a non pinning slider attacking the king
    Side stm = board.getSTM();
    Side xstm = board.getXSTM();
    Square cur_from_square = From(move);
    BitBoard cur_from_square_bb = BitBoard(1) << cur_from_square;
    Square cur_to_square = To(move);
    BitBoard cur_to_square_bb = BitBoard(1) << cur_to_square;
    // BitBoard king_square_stm_bb = board.getPieceBB(makePiece(KING, stm));
    BitBoard king_square_xstm_bb = board.getPieceBB(makePiece(KING, xstm));
    // Square king_square_stm = static_cast<Square>(getLSB(king_square_stm_bb));
    Square king_square_xstm = static_cast<Square>(getLSB(king_square_xstm_bb));
    Piece cur_piece = MovePiece(move);
    bool isep = IsEP(move);
    if (isep) {
        cur_from_square_bb |= stm == WHITE ? shiftSouth(cur_to_square_bb) : shiftNorth(cur_to_square_bb);
    }
    BitBoard occ = board.getOcc(BOTH);
    BitBoard new_occ = (board.getOcc(BOTH) & ~cur_from_square_bb) | cur_to_square_bb;
    // BitBoard new_occ_stm = (board.getOcc(stm) & ~cur_from_square_bb) | cur_to_square_bb;
    BitBoard sliders = (board.getPieceBB(makePiece(BISHOP, stm)) | board.getPieceBB(makePiece(QUEEN, stm))) & getBishopAttacks(king_square_xstm, BitBoard(0)) & getBishopAttacks(cur_from_square, BitBoard(0));
    sliders |= (board.getPieceBB(makePiece(ROOK, stm)) | board.getPieceBB(makePiece(QUEEN, stm))) & getRookAttacks(king_square_xstm, BitBoard(0)) & getRookAttacks(cur_from_square, BitBoard(0));

    // bool pin_mask = false; // contains stm piece pinned by stm piece against xstm king
    while (sliders) {
        Square sq = static_cast<Square>(popLSB(sliders));
        BitBoard blockers = between_squares[king_square_xstm][sq] & occ;
        // check stm piece pinning stm piece against xstm king
        // or ep and exactly 2 blockers between slider and king on ep rank
        if (!multipleActiveBits(blockers) || (isep && bitCount(blockers) == 2 && sliders & getAttackingEPRank(stm))) {
            return true;
        }
    }
    if (Prom(move)) {
        return getPieceAttacks(makePiece(promPiece(move), stm), cur_to_square, new_occ) & king_square_xstm_bb;
    } else {
        return getPieceAttacks(cur_piece, cur_to_square, new_occ) & king_square_xstm_bb;
    }
}
