#include "move.h"

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