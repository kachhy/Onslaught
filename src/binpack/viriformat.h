#ifndef VIRIFORMAT_H
#define VIRIFORMAT_H

#include "core/bitboard.h"
#include "core/types.h"
#include <iostream>

enum class MoveType {
    NORMAL = 0,
    EN_PASSANT = 1,
    CASTLING = 2,
    PROMOTION = 3
};

namespace viriformat {
struct PackedBoard {
    BitBoard occ_bb;
    uint8_t pieces[16];
    uint8_t stm_ep_sq;
    uint8_t halfmove;
    uint16_t fullmove;
    int16_t score;
    uint8_t result;
    uint8_t extra;
};

class Move {
public:
    constexpr Move() noexcept : bits_(0) { }

    constexpr Move(uint8_t from, uint8_t to, MoveType type = MoveType::NORMAL) noexcept : bits_(from | (to << 6) | (static_cast<std::uint16_t>(type) << 14)) { }

    constexpr Move(uint8_t from, uint8_t to, DefaultPiece promo) noexcept :
        bits_(from | (to << 6) | (static_cast<std::uint16_t>(promo) << 12) | (static_cast<std::uint16_t>(MoveType::PROMOTION) << 14)) { }

    constexpr std::uint16_t raw() const noexcept { return bits_; }

private:
    std::uint16_t bits_;
};

inline std::ostream& operator<<(std::ostream& os, const PackedBoard& pb) {
    os.write(reinterpret_cast<const char*>(&pb), sizeof(pb));
    return os;
}

} // namespace viriformat

#endif //  VIRIFORMAT_H