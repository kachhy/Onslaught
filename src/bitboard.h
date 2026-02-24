#ifndef BITBOARD_H
#define BITBOARD_H

#include <iostream>
#include <cstdint>

typedef uint64_t BitBoard;

int getBit(const BitBoard& bitboard, int bit);
void setBit(BitBoard& bitboard, int square);
void printBitboard(const BitBoard& bitboard);

#endif // BOARD_H
