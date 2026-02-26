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

inline int bitCount(const BitBoard& bitboard) {
    return __builtin_popcountll(bitboard);
}

inline int getLSB(const BitBoard& bitboard) {
    return __builtin_ctzll(bitboard); // count trailing zeros
}

inline int getMSB(const BitBoard& bitboard) {
    return 63 ^ __builtin_clzll(bitboard); // count leading zeros
}

inline void setBit(BitBoard& bitboard, int bit) {
    bitboard |= (BitBoard(1) << bit);
}

inline void popBit(BitBoard& bitboard, int bit) {
    bitboard &= ~(BitBoard(1) << bit);
}

inline void flipBit(BitBoard& bitboard, int bit) {
    bitboard ^= ~(BitBoard(1) << bit);
}

inline int getRank(int square) {
    return square >> 3;
}

inline int getFile(int square) {
    return square & 7;
}

inline BitBoard rankMask(int rank) {
    return BitBoard(0xFF) << (rank * 8);
}

inline BitBoard fileMask(int file) {
    return BitBoard(0x0101010101010101ULL) << file;
}

inline BitBoard shiftPawnPushes(const BitBoard& pawns, int side) {
    return (side == WHITE) ? shiftNorth(pawns) : shiftSouth(pawns);
}

inline BitBoard shiftPawnAttacks(const BitBoard& pawns, int side) {
    return (side == WHITE) ? shiftNorthWest(pawns) | shiftNorthEast(pawns) : shiftSouthWest(pawns) | shiftSouthEast(pawns);
}

inline BitBoard shiftPawnCapturesWest(const BitBoard& pawns, int side) {
    return (side == WHITE) ? shiftNorthWest(pawns) : shiftSouthWest(pawns);
}

inline BitBoard shiftPawnCapturesEast(const BitBoard& pawns, int side) {
    return (side == WHITE) ? shiftNorthEast(pawns) : shiftSouthEast(pawns);
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
