#ifndef TERMS_H
#define TERMS_H

#include <cstdint>
#include "types.h"

// Score type
struct Score {
    int32_t value;

    constexpr Score() : value(0) {}
    constexpr explicit Score(int32_t v) : value(v) {}

    // Arithmetic operators
    constexpr Score operator+(Score b) const { return Score(value + b.value); }
    constexpr Score operator-(Score b) const { return Score(value - b.value); }
    constexpr Score operator-(       ) const { return Score(-value); }

    // Compound assignment
    constexpr Score& operator+=(Score b) { value += b.value; return *this; }
    constexpr Score& operator-=(Score b) { value -= b.value; return *this; }

    constexpr bool operator==(Score b) const { return value == b.value; }
    constexpr bool operator!=(Score b) const { return value != b.value; }
};

// Construction and extraction
constexpr Score S(const int16_t mg, const int16_t eg) { return static_cast<Score>((static_cast<uint32_t>(mg) << 16) | static_cast<uint16_t>(eg));}
constexpr int16_t MG(const Score s) { return static_cast<int16_t>(s.value >> 16); }
constexpr int16_t EG(const Score s) { return static_cast<int16_t>(s.value); }
constexpr int16_t T(const Score score, const int phase) { return (MG(score) * phase + EG(score) * (MAX_PHASE - phase)) / MAX_PHASE; } // taper

constexpr Score operator*(Score s, int n) { return S(MG(s) * n, EG(s) * n); }
constexpr Score operator*(int n, Score s) { return S(MG(s) * n, EG(s) * n); }

// Eval parameters
extern const Score material_values[6];
constexpr Score TEMPO = S(26, 0);
constexpr Score BISHOP_PAIR = S(29, 84);
constexpr Score PAWN_PHALANX = S(10, 15);
constexpr Score DOUBLED_PAWNS = S(-10, -40);
constexpr Score PAWN_PROTECTION[] = {S(24, 17), S(5, 20), S(7, 22), S(9, 10), S(-4, 20), S(-30, 25)};
constexpr Score PAWN_CONTROL = S(10, 5);
constexpr Score PASSED_PAWNS[] = {S(0, 0), S(0, 0), S(0, 0), S(10, 12), S(50, 48), S(100, 115), S(285, 205), S(0, 0)};
extern const Score KNIGHT_PAWN_ADJ[9];
extern const Score ROOK_PAWN_ADJ[9];
extern const Score pst[6][64];


#endif // TERMS_H