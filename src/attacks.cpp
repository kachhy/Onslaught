#include "attacks.h"

BitBoard king_attacks[64];

BitBoard generateKingAttacks(Square sq) {
    BitBoard attacks = BitBoard(0);
    BitBoard initial = BitBoard(0);
    setBit(initial, sq);

    attacks |= shiftNorth(initial);
    attacks |= shiftNorthEast(initial);
    attacks |= shiftEast(initial);
    attacks |= shiftSouthEast(initial);
    attacks |= shiftSouth(initial);
    attacks |= shiftSouthWest(initial);
    attacks |= shiftWest(initial);
    attacks |= shiftNorthWest(initial);

    return attacks;
}

void populateKingAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        king_attacks[sq] = generateKingAttacks(static_cast<Square>(sq));
    }
}