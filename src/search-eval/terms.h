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
constexpr Score S(const int16_t mg, const int16_t eg) { return static_cast<Score>(static_cast<int32_t>((static_cast<uint32_t>(mg) << 16) + static_cast<uint32_t>(static_cast<int32_t>(eg)))); }
constexpr int16_t EG(const Score s) { return static_cast<int16_t>(s.value); }
constexpr int16_t MG(const Score s) { return static_cast<int16_t>(static_cast<int32_t>(static_cast<uint32_t>(s.value) + 0x8000u) >> 16); }
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
	S(104, 183),
	S(315, 588),
	S(391, 708),
	S(470, 1116),
	S(811, 1102),
	S(0, 0),
};
constexpr Score TEMPO = S(17, 16);
constexpr Score MOBILITY[5] = {
	S(1, 1),
	S(-1, 10),
	S(3, 12),
	S(2, 5),
	S(0, 9),
};

constexpr Score PAWN_PHALANX = S(18, 47);
constexpr Score DOUBLED_PAWNS = S(12, 15);
constexpr Score BACKWARDS_PAWN = S(-6, -15);
constexpr Score PAWN_PROTECTION[6] = {
	S(25, 40),
	S(-2, 23),
	S(6, 36),
	S(-4, 10),
	S(-14, 27),
	S(-29, 13),
};
constexpr Score PASSED_PAWNS[8] = {
	S(192, 281),
	S(76, 190),
	S(31, 118),
	S(-12, 79),
	S(-22, 45),
	S(-13, 34),
	S(37, 26),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(60, 7);
constexpr Score KNIGHT_BEHIND_PAWN = S(14, 30);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(70, 13),
	S(83, 42),
	S(74, 48),
	S(53, 67),
	S(41, 85),
	S(39, 103),
	S(39, 136),
	S(42, 168),
	S(39, 185),
};

constexpr Score BISHOP_PAIR = S(6, 94);
constexpr Score BISHOP_CONTROL_PENALTY = S(0, -14);
constexpr Score BAD_BISHOP = S(-7, -2);
constexpr Score BISHOP_BLOCKING_PAWN = S(-6, -6);
constexpr Score BISHOP_BEHIND_PAWN = S(15, 15);
constexpr Score TRAPPED_BISHOP = S(-3, -3);

constexpr Score ROOK_ON_SEVENTH_RANK = S(43, 60);
constexpr Score ROOK_ON_OPEN_FILE = S(54, 15);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(17, 23);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(136, 77),
	S(84, 85),
	S(61, 107),
	S(42, 123),
	S(37, 137),
	S(30, 153),
	S(26, 165),
	S(16, 178),
	S(3, 203),
};

constexpr Score QUEEN_REL_PIN = S(-20, -3);
constexpr Score NO_OPPONENT_QUEENS = S(600, 959);

constexpr Score KING_ON_OPEN_FILE = S(-67, -23);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-35, 19);
constexpr Score PAWN_SHIELD[4] = {
	S(-31, -3),
	S(2, -17),
	S(-1, 1),
	S(-15, -20),
};
constexpr Score PAWN_STORM[3] = {
	S(-27, 35),
	S(8, -3),
	S(10, -8),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-18, -5),
	S(-14, -1),
	S(-26, 3),
	S(-13, -42),
};
constexpr Score KING_CASTLED[2] = {
	S(-19, -13),
	S(-54, 40),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(28, -40);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(59, -13);
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