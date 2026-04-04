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
	S(148, 233),
	S(382, 694),
	S(464, 822),
	S(542, 1298),
	S(1011, 1341),
	S(0, 0),
};
constexpr Score TEMPO = S(19, 20);
constexpr Score MOBILITY[5] = {
	S(2, 2),
	S(-1, 10),
	S(4, 13),
	S(3, 5),
	S(1, 9),
};

constexpr Score PAWN_PHALANX = S(19, 41);
constexpr Score DOUBLED_PAWNS = S(-15, -64);
constexpr Score BACKWARDS_PAWN = S(-1, 3);
constexpr Score PAWN_PROTECTION[6] = {
	S(27, 37),
	S(-2, 30),
	S(7, 43),
	S(1, 10),
	S(-15, 40),
	S(-44, 27),
};
constexpr Score PASSED_PAWNS[8] = {
	S(205, 276),
	S(88, 204),
	S(37, 115),
	S(-14, 75),
	S(-26, 39),
	S(-20, 27),
	S(78, 56),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(21, -8);
constexpr Score KNIGHT_BEHIND_PAWN = S(12, 31);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(85, 25),
	S(109, 57),
	S(96, 58),
	S(72, 84),
	S(50, 105),
	S(46, 124),
	S(39, 160),
	S(39, 189),
	S(36, 215),
};

constexpr Score BISHOP_PAIR = S(-17, 114);
constexpr Score BISHOP_CONTROL_PENALTY = S(-3, -16);
constexpr Score BAD_BISHOP = S(-8, -3);
constexpr Score BISHOP_BLOCKING_PAWN = S(-11, -10);
constexpr Score TRAPPED_BISHOP = S(11, -158);

constexpr Score ROOK_ON_SEVENTH_RANK = S(37, 73);
constexpr Score ROOK_ON_OPEN_FILE = S(57, 24);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(23, 37);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(150, 99),
	S(100, 92),
	S(86, 112),
	S(55, 131),
	S(44, 153),
	S(30, 177),
	S(22, 200),
	S(3, 235),
	S(-18, 269),
};

constexpr Score QUEEN_REL_PIN = S(-18, -16);
constexpr Score NO_OPPONENT_QUEENS = S(684, 1100);

constexpr Score KING_ON_OPEN_FILE = S(-79, -29);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-37, 36);
constexpr Score PAWN_SHIELD[4] = {
	S(-30, -4),
	S(0, -18),
	S(1, -3),
	S(-17, -27),
};
constexpr Score PAWN_STORM[3] = {
	S(1, 6),
	S(-5, -35),
	S(-6, -17),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-14, 0),
	S(-15, 0),
	S(-31, 4),
	S(-14, -40),
};
constexpr Score KING_CASTLED[2] = {
	S(-20, -19),
	S(-64, 50),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(28, -42);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(64, -23);
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