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
	S(122, 207),
	S(325, 594),
	S(402, 700),
	S(482, 1105),
	S(868, 1150),
	S(0, 0),
};
constexpr Score TEMPO = S(16, 17);
constexpr Score MOBILITY[5] = {
	S(2, 2),
	S(-1, 6),
	S(2, 10),
	S(2, 4),
	S(1, 4),
};

constexpr Score PAWN_PHALANX = S(16, 47);
constexpr Score DOUBLED_PAWNS = S(-3, -23);
constexpr Score BACKWARDS_PAWN = S(-5, -18);
constexpr Score PAWN_PROTECTION[6] = {
	S(22, 40),
	S(-3, 25),
	S(6, 34),
	S(1, 7),
	S(-15, 25),
	S(-37, 14),
};
constexpr Score PASSED_PAWNS[8] = {
	S(187, 277),
	S(67, 192),
	S(32, 115),
	S(-9, 73),
	S(-21, 45),
	S(-12, 33),
	S(50, 36),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(58, 3);
constexpr Score KNIGHT_BEHIND_PAWN = S(14, 27);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(89, 9),
	S(98, 45),
	S(73, 49),
	S(63, 70),
	S(39, 92),
	S(39, 111),
	S(37, 144),
	S(38, 178),
	S(40, 192),
};

constexpr Score BISHOP_PAIR = S(-6, 97);
constexpr Score BISHOP_CONTROL_PENALTY = S(-1, -13);
constexpr Score BAD_BISHOP = S(-8, -2);
constexpr Score BISHOP_BLOCKING_PAWN = S(-5, -8);
constexpr Score BISHOP_BEHIND_PAWN = S(13, 16);
constexpr Score TRAPPED_BISHOP = S(0, -136);

constexpr Score ROOK_ON_SEVENTH_RANK = S(35, 64);
constexpr Score ROOK_ON_OPEN_FILE = S(50, 17);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(14, 25);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(155, 69),
	S(92, 79),
	S(76, 98),
	S(41, 119),
	S(37, 135),
	S(27, 152),
	S(21, 167),
	S(10, 189),
	S(-10, 230),
};

constexpr Score QUEEN_REL_PIN = S(-18, -5);
constexpr Score NO_OPPONENT_QUEENS = S(603, 968);

constexpr Score KING_ON_OPEN_FILE = S(-70, -23);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-33, 16);
constexpr Score PAWN_SHIELD[4] = {
	S(-31, 0),
	S(-2, -10),
	S(-1, 1),
	S(-16, -17),
};
constexpr Score PAWN_STORM[3] = {
	S(-28, 35),
	S(9, -5),
	S(12, -9),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-13, -3),
	S(-13, 0),
	S(-25, 3),
	S(-14, -37),
};
constexpr Score KING_CASTLED[2] = {
	S(-22, -9),
	S(-54, 44),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(32, -42);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(59, -29);
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