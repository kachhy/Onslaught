#ifndef FEATURES_H
#define FEATURES_H

#include "core/types.h"
#include <algorithm>
#include <cstdint>

// Architecture: (INPUT_SIZE -> HIDDEN_SIZE)x2 -> 1
constexpr size_t INPUT_SIZE = 768;
constexpr size_t HIDDEN_SIZE = 32;

// Quantisation scales
constexpr int32_t EVAL_SCALE = 400;
constexpr int32_t L0_SCALE = 255; // feature-transformer activation
constexpr int32_t L1_SCALE = 64;  // output-layer weights
constexpr int32_t MUL_SCALE = L0_SCALE * L1_SCALE;

// Squared clipped ReLU: clamp(x, 0, L0)^2.
constexpr int32_t screlu(int32_t x) {
    const int32_t v = std::clamp(x, 0, L0_SCALE);
    return v * v;
}

// Per-perspective feature index
template <Side perspective>
constexpr inline int featureIndex(DefaultPiece p, Side color, Square sq) {
    const int bullet_sq = static_cast<int>(sq) ^ 56;
    const int c = static_cast<int>(color);

    if constexpr (perspective == WHITE) {
        return c * 384 + p * 64 + bullet_sq;
    } else {
        return (c ^ 1) * 384 + p * 64 + (bullet_sq ^ 56);
    }
}

#endif // FEATURES_H
