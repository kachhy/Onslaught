#include "bitboard.h"

int getBit(const BitBoard& bitboard, int bit) {
    return bitboard & (BitBoard(1) << bit);
}

void setBit(BitBoard& bitboard, int square) {
    bitboard |= (BitBoard(1) << square);
}

void printBitboard(const BitBoard& bitboard) {
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            std::cout << "\n" << (i / 8) << " ";
        }
        std::cout << getBit(bitboard, i);
    }
    std::cout << "\n  abcdefgh\n";
}

// int main() {
//     BitBoard bb = 0;
//     printBitboard(bb);
// }