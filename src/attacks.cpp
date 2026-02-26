#include "attacks.h"

BitBoard king_attacks[64];
BitBoard knight_attacks[64];

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

BitBoard generateKnightAttacks(Square sq) {
    BitBoard attacks = BitBoard(0);
    BitBoard initial = BitBoard(0);
    setBit(initial, sq);

    if ((initial >> 17) & ~H_FILE)
        attacks |= (initial >> 17);
    if ((initial >> 15) & ~A_FILE)
        attacks |= (initial >> 15);
    if ((initial >> 10) & ~(G_FILE | H_FILE))
        attacks |= (initial >> 10);
    if ((initial >> 6) & ~(A_FILE | B_FILE))
        attacks |= (initial >> 6);

    if ((initial << 17) & ~A_FILE)
        attacks |= (initial << 17);
    if ((initial << 15) & ~H_FILE)
        attacks |= (initial << 15);
    if ((initial << 10) & ~(A_FILE | B_FILE))
        attacks |= (initial << 10);
    if ((initial << 6) & ~(G_FILE | H_FILE))
        attacks |= (initial << 6);

    return attacks;
}

void populateKingAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        king_attacks[sq] = generateKingAttacks(static_cast<Square>(sq));
    }
}

void populateKnightAttacks() {
    for (uint8_t sq = 0; sq < 64; sq++) {
        knight_attacks[sq] = generateKnightAttacks(static_cast<Square>(sq));
    }
}