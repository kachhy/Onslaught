#include "bitboard.h"

inline BitBoard shiftNorth(const BitBoard& bitboard) {
    return bitboard >> 8;
}

inline BitBoard shiftNorthEast(const BitBoard& bitboard) {
    return (bitboard & ~H_FILE) >> 7;
}

inline BitBoard shiftEast(const BitBoard& bitboard) {
    return (bitboard & ~H_FILE) << 1;
}

inline BitBoard shiftSouthEast(const BitBoard& bitboard) {
    return (bitboard & ~H_FILE) << 9;
}

inline BitBoard shiftSouth(const BitBoard& bitboard) {
    return bitboard << 8;
}

inline BitBoard shiftSouthWest(const BitBoard& bitboard) {
    return (bitboard & ~A_FILE) << 7;
}

inline BitBoard shiftWest(const BitBoard& bitboard) {
    return (bitboard & ~A_FILE) >> 1;
}

inline BitBoard shiftNorthWest(const BitBoard& bitboard) {
    return (bitboard & ~A_FILE) >> 9;
}

inline int getBit(const BitBoard& bitboard, int bit) {
    return static_cast<int>((bitboard >> bit) & BitBoard(1));
}

inline void setBit(BitBoard& bitboard, int square) {
    bitboard |= (BitBoard(1) << square);
}

inline int getRank(int square) {
    return square >> 3;
}

inline int getFile(int square) {
    return square & 7;
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