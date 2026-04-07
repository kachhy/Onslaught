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
	S(145, 231),
	S(368, 672),
	S(460, 798),
	S(535, 1260),
	S(981, 1300),
	S(0, 0),
};
constexpr Score TEMPO = S(18, 19);
constexpr Score MOBILITY[5] = {
	S(2, 2),
	S(-1, 8),
	S(3, 12),
	S(3, 4),
	S(1, 5),
};

constexpr Score PAWN_PHALANX = S(17, 42);
constexpr Score DOUBLED_PAWNS = S(-14, -62);
constexpr Score BACKWARDS_PAWN = S(-4, -16);
constexpr Score PAWN_PROTECTION[6] = {
	S(23, 34),
	S(-4, 26),
	S(7, 39),
	S(1, 5),
	S(-16, 29),
	S(-31, 13),
};
constexpr Score PASSED_PAWNS[8] = {
	S(214, 310),
	S(82, 200),
	S(37, 119),
	S(-13, 75),
	S(-27, 40),
	S(-17, 29),
	S(65, 46),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(62, 3);
constexpr Score KNIGHT_BEHIND_PAWN = S(12, 29);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(83, 23),
	S(96, 58),
	S(82, 61),
	S(65, 83),
	S(48, 103),
	S(47, 120),
	S(42, 154),
	S(44, 184),
	S(43, 211),
};

constexpr Score BISHOP_PAIR = S(-12, 109);
constexpr Score BISHOP_CONTROL_PENALTY = S(-2, -16);
constexpr Score BAD_BISHOP = S(-9, -3);
constexpr Score BISHOP_BLOCKING_PAWN = S(-12, -11);
constexpr Score TRAPPED_BISHOP = S(9, -158);

constexpr Score ROOK_ON_SEVENTH_RANK = S(38, 70);
constexpr Score ROOK_ON_OPEN_FILE = S(56, 23);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(20, 37);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(152, 97),
	S(97, 92),
	S(81, 111),
	S(51, 130),
	S(43, 148),
	S(30, 170),
	S(23, 190),
	S(8, 223),
	S(-9, 255),
};

constexpr Score QUEEN_REL_PIN = S(-19, -15);
constexpr Score NO_OPPONENT_QUEENS = S(688, 1089);

constexpr Score KING_ON_OPEN_FILE = S(-77, -23);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-35, 29);
constexpr Score PAWN_SHIELD[4] = {
	S(-31, -1),
	S(-1, -14),
	S(-2, -1),
	S(-20, -25),
};
constexpr Score PAWN_STORM[3] = {
	S(-33, 38),
	S(10, -6),
	S(13, -10),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-16, -3),
	S(-14, -1),
	S(-29, 4),
	S(-16, -41),
};
constexpr Score KING_CASTLED[2] = {
	S(-23, -14),
	S(-59, 45),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(34, -47);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(61, -15);
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