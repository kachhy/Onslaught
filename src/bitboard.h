#ifndef BITBOARD_H
#define BITBOARD_H

#include <iostream>
#include <stdlib.h>
#include <cstdint>
#include "types.h"

using BitBoard = uint64_t;

constexpr int8_t NORTH = -8;
constexpr int8_t SOUTH = 8;
constexpr int8_t EAST = 1;
constexpr int8_t WEST = -1;
constexpr int8_t NORTHEAST = -7;
constexpr int8_t NORTHWEST = -9;
constexpr int8_t SOUTHEAST = 9;
constexpr int8_t SOUTHWEST = 7;

constexpr BitBoard A_FILE = 0x0101010101010101ULL;
constexpr BitBoard B_FILE = 0x0202020202020202ULL;
constexpr BitBoard C_FILE = 0x0404040404040404ULL;
constexpr BitBoard D_FILE = 0x0808080808080808ULL;
constexpr BitBoard E_FILE = 0x1010101010101010ULL;
constexpr BitBoard F_FILE = 0x2020202020202020ULL;
constexpr BitBoard G_FILE = 0x4040404040404040ULL;
constexpr BitBoard H_FILE = 0x8080808080808080ULL;

constexpr BitBoard RANK_1 = 0xFF00000000000000ULL;
constexpr BitBoard RANK_2 = 0x00FF000000000000ULL;
constexpr BitBoard RANK_3 = 0x0000FF0000000000ULL;
constexpr BitBoard RANK_4 = 0x000000FF00000000ULL;
constexpr BitBoard RANK_5 = 0x00000000FF000000ULL;
constexpr BitBoard RANK_6 = 0x0000000000FF0000ULL;
constexpr BitBoard RANK_7 = 0x000000000000FF00ULL;
constexpr BitBoard RANK_8 = 0x00000000000000FFULL;

constexpr inline BitBoard shiftNorth(const BitBoard& bitboard) { return bitboard >> 8; }
constexpr inline BitBoard shiftNorthEast(const BitBoard& bitboard) { return (bitboard & ~H_FILE) >> 7; }
constexpr inline BitBoard shiftEast(const BitBoard& bitboard) { return (bitboard & ~H_FILE) << 1; }
constexpr inline BitBoard shiftSouthEast(const BitBoard& bitboard) { return (bitboard & ~H_FILE) << 9; }
constexpr inline BitBoard shiftSouth(const BitBoard& bitboard) { return bitboard << 8; }
constexpr inline BitBoard shiftSouthWest(const BitBoard& bitboard) { return (bitboard & ~A_FILE) << 7; }
constexpr inline BitBoard shiftWest(const BitBoard& bitboard) { return (bitboard & ~A_FILE) >> 1; }
constexpr inline BitBoard shiftNorthWest(const BitBoard& bitboard) { return (bitboard & ~A_FILE) >> 9; }

constexpr inline int getBit(const BitBoard& bitboard, int bit) { return static_cast<int>((bitboard >> bit) & BitBoard(1)); }
constexpr inline int bitCount(const BitBoard& bitboard) { return __builtin_popcountll(bitboard); }
constexpr inline int getLSB(const BitBoard& bitboard) { return __builtin_ctzll(bitboard); }
constexpr inline int getMSB(const BitBoard& bitboard) { return 63 ^ __builtin_clzll(bitboard); }
constexpr inline int popLSB(BitBoard& bitboard) { int lsb = getLSB(bitboard); bitboard &= bitboard - 1; return lsb; }

void setBit(BitBoard& bitboard, int bit);
void popBit(BitBoard& bitboard, int bit);
void flipBit(BitBoard& bitboard, int bit);
void flipBits(BitBoard& bitboard, int bit_1, int bit_2);

constexpr int getRank(int square) { return square >> 3; }
constexpr int getFile(int square) { return square & 7; }

constexpr BitBoard getRankMask(int rank) { return BitBoard(0xFF) << (rank * 8); }
constexpr BitBoard getFileMask(int file) { return BitBoard(0x0101010101010101ULL) << file; }

constexpr BitBoard shiftPawnPushes(const BitBoard& pawns, int side) { return (side == WHITE) ? shiftNorth(pawns) : shiftSouth(pawns); }
constexpr BitBoard shiftPawnAttacks(const BitBoard& pawns, int side) { return (side == WHITE) ? shiftNorthWest(pawns) | shiftNorthEast(pawns) : shiftSouthWest(pawns) | shiftSouthEast(pawns); }
constexpr BitBoard shiftPawnCapturesWest(const BitBoard& pawns, int side) { return (side == WHITE) ? shiftNorthWest(pawns) : shiftSouthWest(pawns); }
constexpr BitBoard shiftPawnCapturesEast(const BitBoard& pawns, int side) { return (side == WHITE) ? shiftNorthEast(pawns) : shiftSouthEast(pawns); }

void printBitboard(const BitBoard& bitboard);

#endif // BOARD_H
