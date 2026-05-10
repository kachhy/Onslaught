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
        S(54, 115),
        S(231, 430),
        S(289, 526),
        S(345, 864),
        S(521, 723),
        S(0, 0),
};
constexpr Score TEMPO = S(14, 15);
constexpr Score MOBILITY[5] = {
        S(0, 0),
        S(0, 17),
        S(2, 15),
        S(2, 7),
        S(-2, 21),
};

constexpr Score PAWN_PHALANX = S(16, 38);
constexpr Score DOUBLED_PAWNS = S(31, 73);
constexpr Score BACKWARDS_PAWN = S(-5, -13);
constexpr Score PAWN_PROTECTION[6] = {
        S(22, 31),
        S(0, 15),
        S(5, 30),
        S(-4, 10),
        S(-14, 36),
        S(-17, 11),
};
constexpr Score PASSED_PAWNS[8] = {
        S(149, 226),
        S(74, 160),
        S(29, 96),
        S(-10, 62),
        S(-19, 30),
        S(-13, 23),
        S(9, 7),
        S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(53, 14);
constexpr Score KNIGHT_BEHIND_PAWN = S(13, 25);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
        S(26, 16),
        S(39, 31),
        S(47, 29),
        S(34, 38),
        S(26, 48),
        S(25, 64),
        S(27, 88),
        S(31, 112),
        S(32, 89),
};

constexpr Score BISHOP_PAIR = S(16, 69);
constexpr Score BISHOP_CONTROL_PENALTY = S(1, -12);
constexpr Score BAD_BISHOP = S(-6, 6);
constexpr Score BISHOP_BLOCKING_PAWN = S(-6, -6);
constexpr Score BISHOP_BEHIND_PAWN = S(13, 8);

constexpr Score ROOK_ON_SEVENTH_RANK = S(37, 50);
constexpr Score ROOK_ON_OPEN_FILE = S(48, 11);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(15, 18);
constexpr Score ROOK_PAWN_ADJ[9] = {
        S(67, 83),
        S(49, 87),
        S(30, 103),
        S(26, 107),
        S(27, 109),
        S(30, 112),
        S(34, 114),
        S(35, 113),
        S(34, 90),
};

constexpr Score QUEEN_REL_PIN = S(-19, 11);
constexpr Score NO_OPPONENT_QUEENS = S(456, 680);

constexpr Score KING_ON_OPEN_FILE = S(-58, -19);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-28, 14);
constexpr Score PAWN_SHIELD[4] = {
        S(-22, -8),
        S(7, -19),
        S(2, -3),
        S(-10, -20),
};
constexpr Score PAWN_STORM[3] = {
        S(-24, 32),
        S(9, -4),
        S(10, -7),
};
constexpr Score KING_ZONE_ATTACK[4] = {
        S(-20, -4),
        S(-13, -1),
        S(-22, 1),
        S(-10, -42),
};
constexpr Score KING_CASTLED[2] = {
        S(-8, -17),
        S(-43, 28),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(13, -15);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(43, -2);
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