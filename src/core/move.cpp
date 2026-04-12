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
    if (Prom(move)) {
        return getPieceAttacks(makePiece(promPiece(move), board.getSTM()), To(move), board.getOcc(BOTH)) & board.getPieceBB(makePiece(KING, board.getXSTM()));
    }
    return getPieceAttacks(MovePiece(move), To(move), board.getOcc(BOTH)) & board.getPieceBB(makePiece(KING, board.getXSTM()));
}
