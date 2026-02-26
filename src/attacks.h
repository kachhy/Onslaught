#ifndef ATTACKS_H
#define ATTACKS_H

#include "bitboard.h"

extern BitBoard king_attacks[64];

// OTF attack generation
BitBoard generateKingAttacks(Square sq);


// Population functions
void populateKingAttacks();

#endif // ATTACKS_H