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
    S(86, 159), S(303, 554), S(376, 671), S(459, 1061), S(743, 1030), S(0, 0),
};
constexpr Score TEMPO = S(14, 19);
constexpr Score MOBILITY[5] = {
    S(1, 1), S(-1, 10), S(2, 12), S(2, 5), S(-1, 11),
};

constexpr Score PAWN_PHALANX = S(16, 42);
constexpr Score DOUBLED_PAWNS = S(21, 40);
constexpr Score BACKWARDS_PAWN = S(-5, -16);
constexpr Score ISOLATED_PAWN = S(-3, -10);
constexpr Score PAWN_PROTECTION[6] = {
    S(24, 36), S(0, 19), S(4, 35), S(-5, 8), S(-15, 31), S(-36, 17),
};
constexpr Score PASSED_PAWNS[8] = {
    S(171, 269), S(67, 187), S(24, 114), S(-9, 73), S(-25, 38), S(-12, 30), S(26, 18), S(0, 0),
};

constexpr Score KNIGHT_OUTPOST = S(56, 11);
constexpr Score KNIGHT_BEHIND_PAWN = S(14, 32);
constexpr Score KNIGHT_PAWN_ADJ[9] = {
    S(54, 15), S(66, 45), S(66, 47), S(47, 62), S(40, 75), S(40, 94), S(43, 122), S(43, 156), S(41, 154),
};

constexpr Score BISHOP_PAIR = S(10, 83);
constexpr Score BISHOP_CONTROL_PENALTY = S(0, -13);
constexpr Score BAD_BISHOP = S(-6, 0);
constexpr Score BISHOP_BLOCKING_PAWN = S(-7, -4);
constexpr Score BISHOP_BEHIND_PAWN = S(13, 13);
constexpr Score TRAPPED_BISHOP = S(0, -65);

constexpr Score ROOK_ON_SEVENTH_RANK = S(45, 57);
constexpr Score ROOK_ON_OPEN_FILE = S(52, 15);
constexpr Score ROOK_ON_SEMI_OPEN_FILE = S(16, 22);
constexpr Score ROOK_PAWN_ADJ[9] = {
    S(123, 91), S(81, 92), S(43, 115), S(37, 122), S(33, 128), S(29, 140), S(28, 153), S(27, 161), S(26, 149),
};

constexpr Score QUEEN_REL_PIN = S(-19, 4);
constexpr Score NO_OPPONENT_QUEENS = S(579, 921);

constexpr Score KING_ON_OPEN_FILE = S(-66, -21);
constexpr Score KING_ON_SEMI_OPEN_FILE = S(-34, 14);
constexpr Score PAWN_SHIELD[4] = {
    S(-27, -4),
    S(5, -18),
    S(-2, 1),
    S(-13, -19),
};
constexpr Score PAWN_STORM[3] = {
    S(-23, 33),
    S(11, -5),
    S(15, -9),
};
constexpr Score KING_ZONE_ATTACK[4] = {
    S(-19, -4),
    S(-15, 0),
    S(-27, 3),
    S(-13, -42),
};
constexpr Score KING_CASTLED[2] = {
    S(-15, -13),
    S(-48, 42),
};
constexpr Score KING_LOST_ONE_CASTLING_RIGHT = S(21, -32);
constexpr Score KING_UNCASTLED_RIGHTS_REMAIN = S(54, -18);
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