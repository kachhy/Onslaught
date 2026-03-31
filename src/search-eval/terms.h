#ifndef TERMS_H
#define TERMS_H

#include "core/bitboard.h"

// Score type
struct Score {
    int32_t value;

    constexpr Score() : value(0) { }
    constexpr explicit Score(int32_t v) : value(v) { }

    // Arithmetic operators
    constexpr Score operator+(Score b) const { return Score(value + b.value); }
    constexpr Score operator-(Score b) const { return Score(value - b.value); }
    constexpr Score operator-() const { return S(-MG(*this), -EG(*this)); }

    // Compound assignment
    constexpr Score& operator+=(Score b) {
        value += b.value;
        return *this;
    }
    constexpr Score& operator-=(Score b) {
        value -= b.value;
        return *this;
    }

    constexpr bool operator==(Score b) const { return value == b.value; }
    constexpr bool operator!=(Score b) const { return value != b.value; }

    constexpr bool operator>(Score b) const { return value > b.value; }
    constexpr bool operator<(Score b) const { return value < b.value; }
    constexpr bool operator>=(Score b) const { return value >= b.value; }
    constexpr bool operator<=(Score b) const { return value <= b.value; }
};

// Construction and extraction
constexpr Score S(const int16_t mg, const int16_t eg) { return static_cast<Score>((static_cast<uint32_t>(mg) << 16) | static_cast<uint16_t>(eg)); }
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
	S(93, 106),
	S(337, 289),
	S(362, 305),
	S(489, 525),
	S(1013, 925),
	S(0, 0),
};
constexpr Score TEMPO = S(14, 10);
constexpr Score MOBILITY[5] = {
	S(7, 6),
	S(2, 10),
	S(7, 10),
	S(7, 13),
	S(2, 5),
};

constexpr Score PAWN_PHALANX = S(12, 27);
constexpr Score DOUBLED_PAWNS = S(1, -27);
constexpr Score BACKWARDS_PAWN = S(-5, -5);
constexpr Score PAWN_PROTECTION[6] = {
	S(20, 28),
	S(-4, 27),
	S(7, 29),
	S(12, 22),
	S(-11, 28),
	S(-17, 35),
};
constexpr Score PASSED_PAWNS[8] = {
	S(14, 14),
	S(12, 14),
	S(2, 12),
	S(-1, 20),
	S(50, 48),
	S(100, 115),
	S(285, 205),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(30, -8);
constexpr Score KNIGHT_BEHIND_PAWN = S(12, 25);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(-31, -31),
	S(-22, -24),
	S(-15, -21),
	S(-6, -6),
	S(-4, 5),
	S(2, 15),
	S(16, 25),
	S(18, 30),
	S(15, 30),
};

constexpr Score BISHOP_PAIR = S(20, 75);
constexpr Score BISHOP_CONTROL_PENALTY = S(-1, 0);
constexpr Score BAD_BISHOP = S(-3, -3);
constexpr Score BISHOP_BLOCKING_PAWN = S(-17, 7);
constexpr Score TRAPPED_BISHOP = S(-242, -185);

constexpr Score ROOK_ON_SEVENTH_RANK = S(8, 56);
constexpr Score ROOK_ON_OPEN_FILE = S(26, 15);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(16, -6);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(5, 15),
	S(2, 9),
	S(0, 21),
	S(-3, 21),
	S(10, 13),
	S(9, 6),
	S(3, 1),
	S(3, -5),
	S(-6, -14),
};

constexpr Score QUEEN_REL_PIN = S(-15, 2);
constexpr Score NO_OPPONENT_QUEENS = S(115, 185);

constexpr Score KING_ON_OPEN_FILE = S(-50, -14);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-25, -7);
constexpr Score PAWN_SHIELD[4] = {
	S(-22, -7),
	S(5, 9),
	S(0, 5),
	S(6, -19),
};
constexpr Score PAWN_STORM[3] = {
	S(15, -8),
	S(-10, -9),
	S(-14, -1),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-20, -10),
	S(-4, 6),
	S(-27, -7),
	S(-64, -25),
};
constexpr Score KING_ZONE_WEAK_SQUARE = S(5, -5);
constexpr Score KING_ZONE_WEAK_SQUARE_EXTENDED = S(-8, -1);
constexpr Score KING_CASTLED[2] = {
	S(7, 7),
	S(-41, -3),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(-36, -8);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(-9, -6);
extern const Score pst[12][64];

constexpr int MVV_LVA[6][6] = {
    // attacker:P       N       B       R       Q       K
    { 15, 14, 13, 12, 11, 10 }, // P
    { 25, 24, 23, 22, 21, 20 }, // N
    { 35, 34, 33, 32, 31, 30 }, // B
    { 45, 44, 43, 42, 41, 40 }, // R
    { 55, 54, 53, 52, 51, 50 }, // Q
    { 0, 0, 0, 0, 0, 0 },       // K should not reach here hopefully lol
};

#endif // TERMS_H