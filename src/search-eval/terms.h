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
	S(112, 192),
	S(340, 620),
	S(424, 737),
	S(506, 1169),
	S(901, 1195),
	S(0, 0),
};
constexpr Score TEMPO = S(17, 18);
constexpr Score MOBILITY[5] = {
	S(2, 2),
	S(-1, 7),
	S(2, 12),
	S(2, 4),
	S(1, 5),
};

constexpr Score PAWN_PHALANX = S(17, 49);
constexpr Score DOUBLED_PAWNS = S(10, 5);
constexpr Score BACKWARDS_PAWN = S(-5, -19);
constexpr Score PAWN_PROTECTION[6] = {
	S(23, 42),
	S(-3, 26),
	S(6, 36),
	S(0, 8),
	S(-16, 28),
	S(-37, 14),
};
constexpr Score PASSED_PAWNS[8] = {
	S(195, 289),
	S(72, 202),
	S(34, 122),
	S(-9, 78),
	S(-23, 47),
	S(-13, 35),
	S(41, 29),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(61, 4);
constexpr Score KNIGHT_BEHIND_PAWN = S(14, 28);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(94, 12),
	S(106, 49),
	S(80, 53),
	S(70, 75),
	S(44, 99),
	S(44, 119),
	S(41, 154),
	S(42, 190),
	S(45, 200),
};

constexpr Score BISHOP_PAIR = S(-5, 103);
constexpr Score BISHOP_CONTROL_PENALTY = S(-1, -14);
constexpr Score BAD_BISHOP = S(-9, -1);
constexpr Score BISHOP_BLOCKING_PAWN = S(-5, -9);
constexpr Score BISHOP_BEHIND_PAWN = S(14, 17);
constexpr Score TRAPPED_BISHOP = S(-4, -4);

constexpr Score ROOK_ON_SEVENTH_RANK = S(38, 68);
constexpr Score ROOK_ON_OPEN_FILE = S(54, 18);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(15, 27);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(159, 76),
	S(97, 85),
	S(79, 106),
	S(43, 126),
	S(40, 144),
	S(30, 160),
	S(24, 175),
	S(13, 197),
	S(-7, 239),
};

constexpr Score QUEEN_REL_PIN = S(-19, -4);
constexpr Score NO_OPPONENT_QUEENS = S(654, 1031);

constexpr Score KING_ON_OPEN_FILE = S(-75, -25);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-36, 17);
constexpr Score PAWN_SHIELD[4] = {
	S(-33, 0),
	S(-1, -12),
	S(0, 0),
	S(-16, -19),
};
constexpr Score PAWN_STORM[3] = {
	S(-31, 37),
	S(9, -5),
	S(12, -10),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-15, -3),
	S(-13, 0),
	S(-27, 3),
	S(-14, -40),
};
constexpr Score KING_CASTLED[2] = {
	S(-21, -11),
	S(-56, 47),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(34, -42);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(63, -32);
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