#ifndef MOVE_H
#define MOVE_H

#include "types.h"
#include <string>

using Move = uint32_t;

// Move flags
constexpr uint8_t QUIET_FLAG        = 0b0000;
constexpr uint8_t CASTLE_FLAG       = 0b0001;
constexpr uint8_t CAPTURE_FLAG      = 0b0100;
constexpr uint8_t EP_FLAG           = 0b0110;
constexpr uint8_t PROMO_FLAG        = 0b1000;
constexpr uint8_t KNIGHT_PROMO_FLAG = 0b1000;
constexpr uint8_t BISHOP_PROMO_FLAG = 0b1001;
constexpr uint8_t ROOK_PROMO_FLAG   = 0b1010;
constexpr uint8_t QUEEN_PROMO_FLAG  = 0b1011;

// Move encoding/decoding
constexpr Move NO_MOVE = 0;
constexpr inline Move GenerateMove(Square from, Square to, Piece piece, uint32_t flags) {
    return (static_cast<Move>(from)) | (static_cast<Move>(to) << 6) | (static_cast<Move>(piece) << 12) | (static_cast<Move>(flags) << 16);
}
constexpr inline Square From(Move move) { return static_cast<Square>((static_cast<int>(move) & 0x0003f) >> 0); }
constexpr inline Square To(Move move) { return static_cast<Square>((static_cast<int>(move) & 0x00fc0) >> 6); }
constexpr inline Piece MovePiece(Move move) { return static_cast<Piece>((static_cast<int>(move) & 0x0f000) >> 12); }
constexpr inline uint32_t Flags(Move move) { return ((static_cast<uint32_t>(move) & 0x000f0000u) >> 16); }
constexpr inline bool Capture(Move move) { return (Flags(move) & CAPTURE_FLAG) != 0; }
constexpr inline bool IsEP(Move move) { return Flags(move) == EP_FLAG; }
constexpr inline bool Castle(Move move) { return Flags(move) == CASTLE_FLAG; }
constexpr inline bool Prom(Move move) { return (Flags(move) & PROMO_FLAG) != 0; }
class Board;
std::string moveToStr(Move move);
Move strToMove(const std::string& str, const Board& board);
DefaultPiece promPiece(Move move);
bool givesCheck(const Board& board, Move move);

#endif // MOVE_H
