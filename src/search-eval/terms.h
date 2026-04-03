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
    constexpr Score operator-() const;

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
constexpr Score Score::operator-() const { return S(-MG(*this), -EG(*this)); }

// Eval masks
constexpr BitBoard WHITE_OUTPOST_RANKS = RANK_4 | RANK_5 | RANK_6;
constexpr BitBoard BLACK_OUTPOST_RANKS = RANK_3 | RANK_4 | RANK_5;

constexpr BitBoard G1_H1 = (1ULL << G1) | (1ULL << H1);
constexpr BitBoard A1_B1_C1 = (1ULL << A1) | (1ULL << B1) | (1ULL << C1);
constexpr BitBoard G8_H8 = (1ULL << G8) | (1ULL << H8);
constexpr BitBoard A8_B8_C8 = (1ULL << A8) | (1ULL << B8) | (1ULL << C8);

// Eval parameters
constexpr Score material_values[6] = {
	S(134, 244),
	S(328, 573),
	S(387, 668),
	S(455, 1057),
	S(909, 1180),
	S(0, 0),
};
constexpr Score TEMPO = S(15, 17);
constexpr Score MOBILITY[5] = {
	S(3, 3),
	S(-1, 6),
	S(3, 10),
	S(3, 3),
	S(1, 5),
};

constexpr Score PAWN_PHALANX = S(15, 26);
constexpr Score DOUBLED_PAWNS = S(-10, -60);
constexpr Score BACKWARDS_PAWN = S(-1, 2);
constexpr Score PAWN_PROTECTION[6] = {
	S(22, 26),
	S(-2, 24),
	S(5, 33),
	S(2, 8),
	S(-11, 27),
	S(-46, 24),
};
constexpr Score PASSED_PAWNS[8] = {
	S(34, 79),
	S(-8, 46),
	S(-19, 18),
	S(-13, 3),
	S(22, 21),
	S(44, 50),
	S(125, 90),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(18, -10);
constexpr Score KNIGHT_BEHIND_PAWN = S(10, 25);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(86, 5),
	S(88, 40),
	S(67, 45),
	S(51, 65),
	S(32, 82),
	S(28, 99),
	S(21, 128),
	S(21, 153),
	S(23, 162),
};

constexpr Score BISHOP_PAIR = S(-16, 92);
constexpr Score BISHOP_CONTROL_PENALTY = S(-3, -14);
constexpr Score BAD_BISHOP = S(-7, -1);
constexpr Score BISHOP_BLOCKING_PAWN = S(-11, -9);
constexpr Score TRAPPED_BISHOP = S(7, -105);

constexpr Score ROOK_ON_SEVENTH_RANK = S(17, 65);
constexpr Score ROOK_ON_OPEN_FILE = S(46, 21);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(19, 28);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(130, 59),
	S(79, 67),
	S(68, 83),
	S(35, 103),
	S(28, 122),
	S(17, 139),
	S(9, 157),
	S(-6, 187),
	S(-29, 235),
};

constexpr Score QUEEN_REL_PIN = S(-16, -9);
constexpr Score NO_OPPONENT_QUEENS = S(464, 832);

constexpr Score KING_ON_OPEN_FILE = S(-66, -22);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-33, 30);
constexpr Score PAWN_SHIELD[4] = {
	S(-27, -1),
	S(-2, -13),
	S(-1, -1),
	S(-16, -21),
};
constexpr Score PAWN_STORM[3] = {
	S(1, 5),
	S(-5, -28),
	S(-8, -13),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-11, 0),
	S(-12, 0),
	S(-23, 3),
	S(-11, -32),
};
constexpr Score KING_CASTLED[2] = {
	S(-22, -12),
	S(-55, 42),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(19, -34);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(49, -34);
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