#ifndef TERMS_H
#define TERMS_H

#include "bitboard.h"

// Score type
struct Score {
    int32_t value;

    constexpr Score() : value(0) {}
    constexpr explicit Score(int32_t v) : value(v) {}

    // Arithmetic operators
    constexpr Score operator+(Score b) const { return Score(value + b.value); }
    constexpr Score operator-(Score b) const { return Score(value - b.value); }
    constexpr Score operator-(       ) const { return Score(-value); }

    // Compound assignment
    constexpr Score& operator+=(Score b) { value += b.value; return *this; }
    constexpr Score& operator-=(Score b) { value -= b.value; return *this; }

    constexpr bool operator==(Score b) const { return value == b.value; }
    constexpr bool operator!=(Score b) const { return value != b.value; }
};

// Construction and extraction
constexpr Score S(const int16_t mg, const int16_t eg) { return static_cast<Score>((static_cast<uint32_t>(mg) << 16) | static_cast<uint16_t>(eg));}
constexpr int16_t MG(const Score s) { return static_cast<int16_t>(s.value >> 16); }
constexpr int16_t EG(const Score s) { return static_cast<int16_t>(s.value); }
constexpr int16_t T(const Score score, const int phase) { return (MG(score) * phase + EG(score) * (MAX_PHASE - phase)) / MAX_PHASE; } // taper

constexpr Score operator*(Score s, int n) { return S(MG(s) * n, EG(s) * n); }
constexpr Score operator*(int n, Score s) { return S(MG(s) * n, EG(s) * n); }

// Eval masks
constexpr BitBoard WHITE_OUTPOST_RANKS = RANK_4 | RANK_5 | RANK_6;
constexpr BitBoard BLACK_OUTPOST_RANKS = RANK_3 | RANK_4 | RANK_5;

constexpr BitBoard G1_H1 = (1ULL << G1) | (1ULL << H1);
constexpr BitBoard A1_B1_C1 = (1ULL << A1) | (1ULL << B1) | (1ULL << C1);
constexpr BitBoard G8_H8 = (1ULL << G8) | (1ULL << H8);
constexpr BitBoard A8_B8_C8 = (1ULL << A8) | (1ULL << B8) | (1ULL << C8);

// Eval parameters
constexpr Score material_values[6] = {
    S(82, 94), // PAWN
    S(337, 281), // KNIGHT
    S(365, 297), // BISHOP
    S(477, 512), // ROOK
    S(1025, 936), // QUEEN
    S(0, 0), // KING (shouldn't hit this case ever)
};
constexpr Score TEMPO = S(26, 0);
constexpr Score MOBILITY[] = {S(7, 6), S(6, 7), S(3, 5), S(4, 2), S(-5, -4)};

constexpr Score PAWN_PHALANX = S(10, 15);
constexpr Score DOUBLED_PAWNS = S(-10, -40);
constexpr Score PAWN_CONTROL = S(10, 5);
constexpr Score PAWN_PROTECTION[] = {S(24, 17), S(5, 20), S(7, 22), S(9, 10), S(-4, 20), S(-30, 25)};
constexpr Score PASSED_PAWNS[] = {S(0, 0), S(0, 0), S(0, 0), S(10, 12), S(50, 48), S(100, 115), S(285, 205), S(0, 0)};

constexpr Score KNIGHT_OUTPOST = S(15, -24);
constexpr Score KNIGHT_BEHIND_PAWN = S(2, 32);

constexpr Score BISHOP_PAIR = S(29, 84);
constexpr Score BISHOP_CONTROL_PENALTY = S(-2, -2);
constexpr Score BAD_BISHOP = S(-10, -15);
constexpr Score BISHOP_BLOCKING_PAWN = S(-20, -1);

constexpr Score ROOK_ON_SEVENTH_FILE = S(0, 50);
constexpr Score ROOK_ON_OPEN_FILE = S(30, 10);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(10, 5);

constexpr Score QUEEN_REL_PIN = S(-19, -10);

constexpr Score NO_OPPONENT_QUEENS = S(128, 196);
constexpr Score KING_ON_OPEN_FILE = S(-36, 0);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-20, 0);
constexpr Score PAWN_SHIELD[4] = {
    S(-36, 0), // missing pawn on this file
    S( 18, 0), // pawn on original rank
    S( 12, 0), // pawn one rank advanced
    S(  5, 0), // pawn two ranks advanced
};
constexpr Score PAWN_STORM[3] = {
    S(-5, 0), // pawn far away
    S(-8, 0), // pawn getting close
    S(-14,0), // pawn on 5th rank or closer
};
constexpr Score KING_ZONE_ATTACK[4] {
    S(-20, -10), // Knight
    S(-20, -10), // Bishop
    S(-40, -20), // Rook
    S(-80, -40) // Queen
};
constexpr Score KING_ZONE_WEAK_SQUARE = S(-10, -5);
constexpr Score KING_ZONE_WEAK_SQUARE_EXTENDED = S(-5, -3);
const Score KING_CASTLED[2] = { 
    S(0, 0),
    S(-50, 0)
};
const Score KING_LOST_ONE_CASTLING_RIGHT = S(-25, 0);
const Score KING_UNCASTLED_RIGHTS_REMAIN = S(-15, 0);

extern const Score KNIGHT_PAWN_ADJ[9];
extern const Score ROOK_PAWN_ADJ[9];
extern const Score pst[6][64];


#endif // TERMS_H