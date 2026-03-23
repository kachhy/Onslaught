#include "bitboard.h"
#include <iostream>

void setBit(BitBoard& bitboard, int bit) { bitboard |= (BitBoard(1) << bit); }

void popBit(BitBoard& bitboard, int bit) { bitboard &= ~(BitBoard(1) << bit); }

void flipBit(BitBoard& bitboard, int bit) { bitboard ^= (BitBoard(1) << bit); }

void flipBits(BitBoard& bitboard, int bit_1, int bit_2) { bitboard ^= (BitBoard(1) << bit_1) | (BitBoard(1) << bit_2); }

void printBitboard(const BitBoard bitboard) {
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            std::cout << "\n" << (8 - (i >> 3)) << " ";
        }
        std::cout << getBit(bitboard, i) << " ";
    }
    std::cout << "\n  a b c d e f g h\n";
}
