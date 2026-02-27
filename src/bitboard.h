#ifndef BITBOARD_H
#define BITBOARD_H

#include <iostream>
#include <stdlib.h>
#include <cstdint>
#include "types.h"

using BitBoard = uint64_t;

constexpr inline int8_t NORTH = -8
constexpr inline int8_t SOUTH = 8
constexpr inline int8_t EAST = 1
constexpr inline int8_t WEST = -1
constexpr inline int8_t NORTHEAST = -7
constexpr inline int8_t NORTHWEST = -9
constexpr inline int8_t SOUTHEAST = 9
constexpr inline int8_t SOUTHWEST = 7

constexpr inline BitBoard A_FILE = 0x0101010101010101ULL;
constexpr inline BitBoard B_FILE = 0x0202020202020202ULL;
constexpr inline BitBoard C_FILE = 0x0404040404040404ULL;
constexpr inline BitBoard D_FILE = 0x0808080808080808ULL;
constexpr inline BitBoard E_FILE = 0x1010101010101010ULL;
constexpr inline BitBoard F_FILE = 0x2020202020202020ULL;
constexpr inline BitBoard G_FILE = 0x4040404040404040ULL;
constexpr inline BitBoard H_FILE = 0x8080808080808080ULL;

constexpr inline BitBoard RANK_1 = 0xFF00000000000000ULL;
constexpr inline BitBoard RANK_2 = 0x00FF000000000000ULL;
constexpr inline BitBoard RANK_3 = 0x0000FF0000000000ULL;
constexpr inline BitBoard RANK_4 = 0x000000FF00000000ULL;
constexpr inline BitBoard RANK_5 = 0x00000000FF000000ULL;
constexpr inline BitBoard RANK_6 = 0x0000000000FF0000ULL;
constexpr inline BitBoard RANK_7 = 0x000000000000FF00ULL;
constexpr inline BitBoard RANK_8 = 0x00000000000000FFULL;

constexpr BitBoard shiftNorth(const BitBoard& bitboard);
constexpr BitBoard shiftNorthEast(const BitBoard& bitboard);
constexpr BitBoard shiftEast(const BitBoard& bitboard);
constexpr BitBoard shiftSouthEast(const BitBoard& bitboard);
constexpr BitBoard shiftSouth(const BitBoard& bitboard);
constexpr BitBoard shiftSouthWest(const BitBoard& bitboard);
constexpr BitBoard shiftWest(const BitBoard& bitboard);
constexpr BitBoard shiftNorthWest(const BitBoard& bitboard);

constexpr int getBit(const BitBoard& bitboard, int bit);
constexpr int bitCount(const BitBoard& bitboard);
constexpr int getLSB(const BitBoard& bitboard);
constexpr int getMSB(const BitBoard& bitboard);

void setBit(BitBoard& bitboard, int bit);
void popBit(BitBoard& bitboard, int bit);
void flipBit(BitBoard& bitboard, int bit);
void flipBits(BitBoard& bitboard, int bit_1, int bit_2);

constexpr int getRank(int square);
constexpr int getFile(int square);

constexpr BitBoard getRankMask(int rank);
constexpr BitBoard getFileMask(int file);

constexpr BitBoard shiftPawnPushes(const BitBoard& pawns, int side);
constexpr BitBoard shiftPawnAttacks(const BitBoard& pawns, int side);
constexpr BitBoard shiftPawnCapturesWest(const BitBoard& pawns, int side);
constexpr BitBoard shiftPawnCapturesEast(const BitBoard& pawns, int side);

void printBitboard(const BitBoard& bitboard);

#endif // BOARD_H
