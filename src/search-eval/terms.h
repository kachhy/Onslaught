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
constexpr Score S(const int16_t mg, const int16_t eg) {
    return static_cast<Score>(static_cast<int32_t>((static_cast<uint32_t>(mg) << 16) + static_cast<uint32_t>(static_cast<int32_t>(eg))));
}
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
	S(105, 144),
	S(245, 512),
	S(340, 627),
	S(390, 978),
	S(650, 975),
	S(0, 0),
};
constexpr Score TEMPO = S(93, 81);
constexpr Score MOBILITY[5] = {
	S(1, 1),
	S(0, 1),
	S(11, 5),
	S(7, -5),
	S(2, 2),
};

constexpr Score PAWN_PHALANX = S(50, 42);
constexpr Score DOUBLED_PAWNS = S(40, 27);
constexpr Score BACKWARDS_PAWN = S(-19, -7);
constexpr Score PAWN_PROTECTION[6] = {
	S(60, 60),
	S(26, 21),
	S(20, 73),
	S(1, 40),
	S(-32, 60),
	S(-56, 0),
};
constexpr Score PASSED_PAWNS[8] = {
	S(131, 221),
	S(51, 139),
	S(35, 93),
	S(-7, 48),
	S(-38, 15),
	S(-46, 12),
	S(26, 18),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(111, 39);
constexpr Score KNIGHT_BEHIND_PAWN = S(27, 30);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(55, 19),
	S(72, 42),
	S(94, 64),
	S(77, 78),
	S(38, 60),
	S(27, 68),
	S(15, 96),
	S(0, 139),
	S(16, 186),
};

constexpr Score BISHOP_PAIR = S(-2, 50);
constexpr Score BISHOP_CONTROL_PENALTY = S(-10, -17);
constexpr Score BAD_BISHOP = S(-21, -7);
constexpr Score BISHOP_BLOCKING_PAWN = S(-9, 17);
constexpr Score BISHOP_BEHIND_PAWN = S(37, 28);

constexpr Score ROOK_ON_SEVENTH_RANK = S(36, 77);
constexpr Score ROOK_ON_OPEN_FILE = S(72, 44);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(41, 25);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(122, 91),
	S(101, 130),
	S(69, 131),
	S(46, 111),
	S(16, 90),
	S(-15, 93),
	S(-3, 115),
	S(51, 178),
	S(66, 204),
};

constexpr Score QUEEN_REL_PIN = S(-42, 17);
constexpr Score NO_OPPONENT_QUEENS = S(488, 868);

constexpr Score KING_ON_OPEN_FILE = S(-111, -14);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-51, 19);
constexpr Score PAWN_SHIELD[4] = {
	S(-37, -16),
	S(22, -11),
	S(-11, -9),
	S(-26, -33),
};
constexpr Score PAWN_STORM[3] = {
	S(-36, 24),
	S(-3, 3),
	S(31, -32),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-21, 8),
	S(-7, 13),
	S(7, 0),
	S(1, -22),
};
constexpr Score KING_CASTLED[2] = {
	S(-6, -21),
	S(-58, 32),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(12, -38);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(52, 13);
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