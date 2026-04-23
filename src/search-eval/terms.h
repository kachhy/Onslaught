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
	S(92, 166),
	S(318, 591),
	S(396, 711),
	S(471, 1121),
	S(800, 1100),
	S(0, 0),
};
constexpr Score TEMPO = S(17, 16);
constexpr Score MOBILITY[5] = {
	S(1, 1),
	S(-1, 10),
	S(2, 11),
	S(2, 5),
	S(0, 8),
};
constexpr Score UNCONTESTED_CENTRAL_CONTROL = S(4, 1);

constexpr Score PAWN_PHALANX = S(17, 25);
constexpr Score DOUBLED_PAWNS = S(20, 35);
constexpr Score BACKWARDS_PAWN = S(-5, -8);
constexpr Score ISOLATED_PAWN = S(-9, -24);
constexpr Score PAWN_PROTECTION[6] = {
	S(24, 18),
	S(-3, 23),
	S(5, 36),
	S(-2, 10),
	S(-14, 27),
	S(-32, 11),
};
constexpr Score PASSED_PAWNS[8] = {
	S(183, 286),
	S(78, 198),
	S(32, 126),
	S(-10, 88),
	S(-17, 53),
	S(-8, 45),
	S(29, 20),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(58, 6);
constexpr Score KNIGHT_BEHIND_PAWN = S(15, 30);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(60, 10),
	S(77, 43),
	S(74, 46),
	S(55, 65),
	S(42, 82),
	S(41, 101),
	S(41, 133),
	S(44, 166),
	S(40, 190),
};

constexpr Score BISHOP_PAIR = S(6, 97);
constexpr Score BISHOP_CONTROL_PENALTY = S(0, -14);
constexpr Score BAD_BISHOP = S(-7, -2);
constexpr Score BISHOP_BLOCKING_PAWN = S(-5, -6);
constexpr Score BISHOP_BEHIND_PAWN = S(14, 15);
constexpr Score TRAPPED_BISHOP = S(-3, -3);

constexpr Score ROOK_ON_SEVENTH_RANK = S(44, 61);
constexpr Score ROOK_ON_OPEN_FILE = S(54, 17);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(18, 23);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(128, 75),
	S(80, 89),
	S(61, 106),
	S(43, 121),
	S(38, 135),
	S(32, 150),
	S(28, 165),
	S(19, 181),
	S(8, 202),
};

constexpr Score QUEEN_REL_PIN = S(-20, -2);
constexpr Score NO_OPPONENT_QUEENS = S(617, 979);

constexpr Score KING_ON_OPEN_FILE = S(-66, -19);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-31, 21);
constexpr Score PAWN_SHIELD[4] = {
	S(-29, -3),
	S(3, -16),
	S(-2, 1),
	S(-15, -22),
};
constexpr Score PAWN_STORM[3] = {
	S(-27, 35),
	S(8, -2),
	S(11, -7),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-17, -5),
	S(-13, -1),
	S(-26, 3),
	S(-13, -42),
};
constexpr Score KING_CASTLED[2] = {
	S(-17, -13),
	S(-52, 41),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(27, -42);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(57, -9);
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