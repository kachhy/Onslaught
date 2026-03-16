#ifndef TERMS_H
#define TERMS_H

#include <cstdint>
#include "types.h"

using Score = int32_t;

constexpr Score S(const int16_t mg, const int16_t eg) { return (static_cast<Score>(mg) << 16) + static_cast<Score>(eg); }
constexpr int16_t MG(const Score score) { return static_cast<int16_t>(score >> 16); }
constexpr int16_t EG(const Score score) { return static_cast<int16_t>(score); }
constexpr Score T(const Score score, const int phase) { return (MG(score) * phase + EG(score) * (MAX_PHASE - phase)) / MAX_PHASE; } // taper

// Eval parameters
extern const Score material_values[6];
constexpr Score TEMPO = S(26, 0);
constexpr Score BISHOP_PAIR = S(29, 84);
extern const Score KNIGHT_PAWN_ADJ[9];
extern const Score ROOK_PAWN_ADJ[9];
extern const int pst[7][64];


#endif // TERMS_H