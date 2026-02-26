#include "bitboard.h"

BitBoard shiftNorth(const BitBoard& bitboard) {
    return bitboard >> 8;
}

BitBoard shiftNorthEast(const BitBoard& bitboard) {
    return (bitboard & ~H_FILE) >> 7;
}

BitBoard shiftEast(const BitBoard& bitboard) {
    return (bitboard & ~H_FILE) << 1;
}

BitBoard shiftSouthEast(const BitBoard& bitboard) {
    return (bitboard & ~H_FILE) << 9;
}

BitBoard shiftSouth(const BitBoard& bitboard) {
    return bitboard << 8;
}

BitBoard shiftSouthWest(const BitBoard& bitboard) {
    return (bitboard & ~A_FILE) << 7;
}

BitBoard shiftWest(const BitBoard& bitboard) {
    return (bitboard & ~A_FILE) >> 1;
}

BitBoard shiftNorthWest(const BitBoard& bitboard) {
    return (bitboard & ~A_FILE) >> 9;
}

int getBit(const BitBoard& bitboard, int bit) {
    return static_cast<int>((bitboard >> bit) & BitBoard(1));
}

int bitCount(const BitBoard& bitboard) {
    return __builtin_popcountll(bitboard);
}

int getLSB(const BitBoard& bitboard) {
    return __builtin_ctzll(bitboard); // count trailing zeros
}

int getMSB(const BitBoard& bitboard) {
    return 63 ^ __builtin_clzll(bitboard); // count leading zeros
}

int popLSB(BitBoard& bitboard) {
    int lsb = getLSB(bitboard);
    bitboard &= bitboard - 1;
    return lsb;
}

void setBit(BitBoard& bitboard, int bit) {
    bitboard |= (BitBoard(1) << bit);
}

void popBit(BitBoard& bitboard, int bit) {
    bitboard &= ~(BitBoard(1) << bit);
}

void flipBit(BitBoard& bitboard, int bit) {
    bitboard ^= (BitBoard(1) << bit);
}

void flipBits(BitBoard& bitboard, int bit_1, int bit_2) {
    bitboard ^= (BitBoard(1) << bit_1) | (BitBoard(1) << bit_2);
}

int getRank(Square square) {
    return square >> 3;
}

int getFile(Square square) {
    return square & 7;
}

BitBoard rankMask(int rank) {
    return BitBoard(0xFF) << (rank * 8);
}

BitBoard fileMask(int file) {
    return BitBoard(0x0101010101010101ULL) << file;
}

BitBoard shiftPawnPushes(const BitBoard& pawns, Side side) {
    return (side == WHITE) ? shiftNorth(pawns) : shiftSouth(pawns);
}

BitBoard shiftPawnAttacks(const BitBoard& pawns, Side side) {
    return (side == WHITE) ? shiftNorthWest(pawns) | shiftNorthEast(pawns) : shiftSouthWest(pawns) | shiftSouthEast(pawns);
}

BitBoard shiftPawnCapturesWest(const BitBoard& pawns, Side side) {
    return (side == WHITE) ? shiftNorthWest(pawns) : shiftSouthWest(pawns);
}

BitBoard shiftPawnCapturesEast(const BitBoard& pawns, Side side) {
    return (side == WHITE) ? shiftNorthEast(pawns) : shiftSouthEast(pawns);
}

void printBitboard(const BitBoard& bitboard) {
    for (int i = 0; i < 64; i++) {
        if (i % 8 == 0) {
            std::cout << "\n" << (8 - (i >> 3)) << " ";
        }
        std::cout << getBit(bitboard, i) << " ";
    }
    std::cout << "\n  a b c d e f g h\n";
}
