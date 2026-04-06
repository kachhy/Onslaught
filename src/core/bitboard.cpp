#include "bitboard.h"
#include <iostream>

void printBitboard(const BitBoard bitboard) {
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            std::cout << "\n" << (8 - (i >> 3)) << " ";
        }
        std::cout << getBit(bitboard, i) << " ";
    }
    std::cout << "\n  a b c d e f g h\n";
}
