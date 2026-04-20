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
	S(127, 220),
	S(337, 616),
	S(424, 733),
	S(498, 1159),
	S(909, 1199),
	S(0, 0),
};
constexpr Score TEMPO = S(17, 18);
constexpr Score MOBILITY[5] = {
	S(2, 2),
	S(-1, 7),
	S(3, 11),
	S(3, 3),
	S(1, 5),
};

constexpr Score PAWN_PHALANX = S(18, 28);
constexpr Score DOUBLED_PAWNS = S(-12, -35);
constexpr Score BACKWARDS_PAWN = S(-3, -14);
constexpr Score PAWN_PROTECTION[6] = {
	S(26, 27),
	S(-4, 26),
	S(6, 39),
	S(2, 8),
	S(-12, 26),
	S(-36, 19),
};
constexpr Score PASSED_PAWNS[8] = {
	S(208, 296),
	S(82, 211),
	S(29, 121),
	S(11, 67),
	S(1, 42),
	S(-4, 45),
	S(56, 40),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(59, 3);
constexpr Score KNIGHT_BEHIND_PAWN = S(11, 26);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(84, 2),
	S(97, 47),
	S(74, 59),
	S(57, 84),
	S(43, 101),
	S(41, 116),
	S(38, 144),
	S(41, 166),
	S(42, 188),
};

constexpr Score BISHOP_PAIR = S(-15, 101);
constexpr Score BISHOP_CONTROL_PENALTY = S(-2, -14);
constexpr Score BAD_BISHOP = S(-8, -3);
constexpr Score BISHOP_BLOCKING_PAWN = S(-12, -6);
constexpr Score BISHOP_BEHIND_PAWN = S(11, 26);
constexpr Score TRAPPED_BISHOP = S(7, -153);

constexpr Score ROOK_ON_SEVENTH_RANK = S(41, 62);
constexpr Score ROOK_ON_OPEN_FILE = S(52, 20);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(17, 26);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(155, 67),
	S(92, 83),
	S(69, 113),
	S(43, 131),
	S(37, 147),
	S(28, 162),
	S(24, 173),
	S(11, 190),
	S(-9, 222),
};

constexpr Score QUEEN_REL_PIN = S(-17, -14);
constexpr Score NO_OPPONENT_QUEENS = S(635, 1007);

constexpr Score KING_ON_OPEN_FILE = S(-72, -25);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-35, 21);
constexpr Score PAWN_SHIELD[4] = {
	S(-31, 1),
	S(-3, -13),
	S(-1, -2),
	S(-16, -21),
};
constexpr Score PAWN_STORM[3] = {
	S(-24, 31),
	S(9, -6),
	S(11, -8),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-15, -2),
	S(-14, 0),
	S(-25, 3),
	S(-14, -38),
};
constexpr Score KING_CASTLED[2] = {
	S(-23, -12),
	S(-55, 45),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(32, -48);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(59, -18);
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