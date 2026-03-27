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
    constexpr Score operator-() const { return Score(-value); }

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
};

// Construction and extraction
constexpr Score S(const int16_t mg, const int16_t eg) { return static_cast<Score>((static_cast<uint32_t>(mg) << 16) | static_cast<uint16_t>(eg)); }
constexpr int16_t MG(const Score s) { return static_cast<int16_t>(s.value >> 16); }
constexpr int16_t EG(const Score s) { return static_cast<int16_t>(s.value); }
constexpr int16_t T(const Score score, const int phase) { return (MG(score) * phase + EG(score) * (MAX_PHASE - phase)) / MAX_PHASE; } // taper

constexpr Score operator*(Score s, int n) { return S(MG(s) * n, EG(s) * n); }
constexpr Score operator*(int n, Score s) { return S(MG(s) * n, EG(s) * n); }

// Eval masks
constexpr BitBoard WHITE_OUTPOST_RANKS = RANK_4 | RANK_5 | RANK_6;
constexpr BitBoard BLACK_OUTPOST_RANKS = RANK_3 | RANK_4 | RANK_5;

constexpr BitBoard G1_H1 = (1ULL << G1) | (1ULL << H1);
constexpr BitBoard A1_B1_C1 = (1ULL << A1) | (1ULL << B1) | (1ULL << C1);
constexpr BitBoard G8_H8 = (1ULL << G8) | (1ULL << H8);
constexpr BitBoard A8_B8_C8 = (1ULL << A8) | (1ULL << B8) | (1ULL << C8);

// Eval parameters
constexpr Score material_values[6] = {
	S(80, 92),
	S(338, 282),
	S(363, 295),
	S(475, 511),
	S(1023, 934),
	S(0, 0),
};
constexpr Score TEMPO = S(21, 1);
constexpr Score MOBILITY[6] = {
	S(7, 6),
	S(6, 7),
	S(2, 4),
	S(4, 2),
	S(-5, -4),
};

constexpr Score PAWN_PHALANX = S(11, 16);
constexpr Score DOUBLED_PAWNS = S(-11, -41);
constexpr Score BACKWARDS_PAWN = S(-20, -10);
constexpr Score PAWN_PROTECTION[8] = {
	S(24, 15),
	S(6, 21),
	S(5, 20),
	S(8, 9),
	S(-3, 18),
	S(-30, 23),
};
constexpr Score PASSED_PAWNS[8] = {
	S(-1, -1),
	S(-1, -1),
	S(0, -1),
	S(10, 12),
	S(50, 48),
	S(100, 115),
	S(285, 205),
	S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(14, -24);
constexpr Score KNIGHT_BEHIND_PAWN = S(3, 33);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
	S(-20, -19),
	S(-15, -14),
	S(-10, -10),
	S(-2, -2),
	S(0, 1),
	S(5, 5),
	S(9, 9),
	S(13, 13),
	S(17, 17),
};

constexpr Score BISHOP_PAIR = S(27, 82);
constexpr Score BISHOP_CONTROL_PENALTY = S(-2, -3);
constexpr Score BAD_BISHOP = S(-11, -16);
constexpr Score BISHOP_BLOCKING_PAWN = S(-19, -2);
constexpr Score TRAPPED_BISHOP = S(-261, -201);

constexpr Score ROOK_ON_SEVENTH_RANK = S(0, 48);
constexpr Score ROOK_ON_OPEN_FILE = S(31, 9);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(8, 6);
constexpr Score ROOK_PAWN_ADJ[9] = {
	S(20, 30),
	S(16, 23),
	S(11, 14),
	S(4, 7),
	S(0, -1),
	S(-6, -9),
	S(-11, -14),
	S(-13, -21),
	S(-19, -29),
};

constexpr Score QUEEN_REL_PIN = S(-19, -10);
constexpr Score NO_OPPONENT_QUEENS = S(126, 194);

constexpr Score KING_ON_OPEN_FILE = S(-36, 0);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-20, 1);
constexpr Score PAWN_SHIELD[4] = {
	S(-36, -1),
	S(19, 0),
	S(10, -1),
	S(3, -1),
};
constexpr Score PAWN_STORM[3] = {
	S(-3, 1),
	S(-7, 0),
	S(-13, 0),
};
constexpr Score KING_ZONE_ATTACK[4] = {
	S(-20, -10),
	S(-20, -8),
	S(-41, -21),
	S(-80, -40),
};
constexpr Score KING_ZONE_WEAK_SQUARE = S(-8, -3);
constexpr Score KING_ZONE_WEAK_SQUARE_EXTENDED = S(-3, -1);
constexpr Score KING_CASTLED[2] = {
	S(0, 0),
	S(-48, 0),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(-26, 0);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(-16, 0);
extern const Score pst[12][64];

#endif // TERMS_H