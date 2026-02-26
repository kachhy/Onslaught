#ifndef ATTACKS_H
#define ATTACKS_H

#include "bitboard.h"

extern BitBoard king_attacks[64];
extern BitBoard knight_attacks[64];

// OTF attack generation
BitBoard generateKingAttacks(Square sq);
BitBoard generateKnightAttacks(Square sq);

// Population functions
void populateKingAttacks();
void populateKnightAttacks();

#endif // ATTACKS_H