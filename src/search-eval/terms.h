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
	S(94, 107),
	S(338, 290),
	S(361, 304),
	S(489, 527),
	S(1014, 926),
	S(0, 0),
};
constexpr Score TEMPO = S(13, 9);
constexpr Score MOBILITY[5] = {
	S(7, 6),
	S(3, 11),
	S(6, 9),
	S(6, 12),
	S(3, 6),
};

constexpr Score PAWN_PHALANX = S(12, 26);
constexpr Score DOUBLED_PAWNS = S(2, -25);
constexpr Score BACKWARDS_PAWN = S(-4, -4);
constexpr Score PAWN_PROTECTION[6] = {
	S(19, 29),
	S(-4, 28),
	S(6, 28),
	S(11, 23),
	S(-10, 29),
	S(-16, 36),
};
constexpr Score PASSED_PAWNS[8] = {
	S(15, 15),
	S(13, 15),
	S(0, 13),
	S(-3, 18),
	S(50, 48),
	S(100, 115),
	S(285, 205),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(31, -7);
constexpr Score KNIGHT_BEHIND_PAWN = S(10, 24);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(-31, -30),
	S(-21, -23),
	S(-14, -20),
	S(-5, -5),
	S(-3, 6),
	S(3, 16),
	S(15, 26),
	S(19, 31),
	S(15, 31),
};

constexpr Score BISHOP_PAIR = S(19, 74);
constexpr Score BISHOP_CONTROL_PENALTY = S(-2, -1);
constexpr Score BAD_BISHOP = S(-2, -3);
constexpr Score BISHOP_BLOCKING_PAWN = S(-15, 6);
constexpr Score TRAPPED_BISHOP = S(-241, -184);

constexpr Score ROOK_ON_SEVENTH_RANK = S(6, 55);
constexpr Score ROOK_ON_OPEN_FILE = S(24, 14);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(17, -7);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(4, 14),
	S(1, 8),
	S(-1, 20),
	S(-4, 20),
	S(9, 15),
	S(10, 7),
	S(5, 2),
	S(4, -4),
	S(-5, -13),
};

constexpr Score QUEEN_REL_PIN = S(-14, 1);
constexpr Score NO_OPPONENT_QUEENS = S(116, 186);

constexpr Score KING_ON_OPEN_FILE = S(-51, -16);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-26, -5);
constexpr Score PAWN_SHIELD[4] = {
	S(-23, -8),
	S(4, 8),
	S(-1, 4),
	S(5, -20),
};
constexpr Score PAWN_STORM[3] = {
	S(14, -9),
	S(-10, -10),
	S(-14, -1),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-19, -9),
	S(-5, 5),
	S(-25, -5),
	S(-63, -24),
};
constexpr Score KING_ZONE_WEAK_SQUARE = S(4, -4);
constexpr Score KING_ZONE_WEAK_SQUARE_EXTENDED = S(-8, 0);
constexpr Score KING_CASTLED[2] = {
	S(6, 6),
	S(-40, -2),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(-35, -7);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(-7, -5);
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