#ifndef FEATURES_H
#define FEATURES_H

#include "core/bitboard.h"
#include "core/types.h"
#include <algorithm>
#include <cstdint>

// Architecture: (INPUT_SIZE -> HIDDEN_SIZE)x2 -> 8
constexpr size_t NUM_KING_BUCKETS = 2;
constexpr size_t NUM_OUTPUT_BUCKETS = 8;
constexpr size_t INPUT_SIZE = 768;
constexpr size_t HIDDEN_SIZE = 1024;
constexpr size_t CPLX_L1 = 32;

// King buckets (Bullet's ChessBucketsMirrored)
constexpr uint8_t KING_BUCKETS[32] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
};

// Horizontal mirror flip files when king is on e-h files
inline bool kingNeedsMirror(int king_sq) { return getFile(king_sq) >= 4; }

// Bucket id from a king square already in the correct perspective orientation
inline int kingBucketIndex(int oriented_king) { return KING_BUCKETS[getRank(oriented_king) * 4 + getFile(oriented_king)]; }

// King-bucket id for a given side
inline int kingBucket(int king_sq, Side persp) {
    int ok = (persp == WHITE) ? (king_sq ^ 56) : king_sq;
    if (kingNeedsMirror(king_sq)) {
        ok ^= 7;
    }
    return kingBucketIndex(ok);
}

// Output bucket from total piece count (Bullet's MaterialCount)
inline int outputBucket(int piece_count) {
    constexpr int DIV = (32 + NUM_OUTPUT_BUCKETS - 1) / NUM_OUTPUT_BUCKETS; // aka 32.div_ceil(N)
    const int b = (piece_count - 2) / DIV;
    return std::clamp(b, 0, static_cast<int>(NUM_OUTPUT_BUCKETS) - 1);
}

// Quantisation scales
constexpr int32_t EVAL_SCALE = 400;
constexpr int32_t L0_SCALE = 255; // feature-transformer activation
constexpr int32_t L1_SCALE = 64;  // output-layer weights
constexpr int32_t MUL_SCALE = L0_SCALE * L1_SCALE;
constexpr int32_t NNUE_WEIGHT_SCALAR = 80;

// Squared clipped ReLU: clamp(x, 0, L0)^2.
constexpr int32_t screlu(int32_t x) {
    const int32_t v = std::clamp(x, 0, L0_SCALE);
    return v * v;
}

// Per-perspective feature index. When the perspective's king is on the e-h files the whole
// board is mirrored horizontally (piece square AND king bucket lookup get file-flipped).
template <Side perspective>
inline int featureIndex(DefaultPiece p, Side color, int sq, int king_sq) {
    const bool mirror = kingNeedsMirror(king_sq);

    int psq, ok;
    if constexpr (perspective == WHITE) {
        psq = sq ^ 56;
        ok = king_sq ^ 56;
    } else {
        psq = sq;
        ok = king_sq;
    }

    if (mirror) {
        psq ^= 7;
        ok ^= 7;
    }

    const int bucket = kingBucketIndex(ok);
    const int c = (perspective == WHITE) ? static_cast<int>(color) : (static_cast<int>(color) ^ 1);
    return bucket * 768 + c * 384 + static_cast<int>(p) * 64 + psq;
}

#endif // FEATURES_H
