#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include "core/bitboard.h"
#include "core/move.h"
#include "features.h"
#include "simd.h"
#include <cstdint>

class Board;

// Network parameters defined in nnue.cpp
extern int16_t network_weights[INPUT_SIZE * NUM_KING_BUCKETS][HIDDEN_SIZE];
extern int16_t network_biases[HIDDEN_SIZE];
extern int16_t output_weights[NUM_OUTPUT_BUCKETS][2 * HIDDEN_SIZE];
extern int16_t output_bias[NUM_OUTPUT_BUCKETS];

class alignas(64) Accumulator {
private:
    int16_t accumulator[2][HIDDEN_SIZE]; // [perspective]
    uint8_t king_sq[2]; // [side]
    bool accumulator_dirty = false;
    Move pending_move; // Move whose delta is owed when accumulator_dirty
public:
    Accumulator() = default;

    void reset() {
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] = network_biases[i];
            accumulator[BLACK][i] = network_biases[i];
        }
        accumulator_dirty = false;
    }

    int32_t evaluate(Side stm, int bucket) const;

    inline bool isDirty() const noexcept { return accumulator_dirty; }

    // Defined in nnue.cpp to break a cycle.
    void refresh(const Board& board);
    void onMove(Move move, const Board& board);
    void update(Move move, const Board& board); // Applies the pending move's delta

    // Apply any owed delta before the board mutates again
    void flush(const Board& board) {
        if (accumulator_dirty) {
            update(pending_move, board);
        }
    }

    template <int N_ADD, int N_SUB>
    inline void apply(int16_t* acc, const int16_t* const* adds, const int16_t* const* subs) {
        for (size_t i = 0; i < HIDDEN_SIZE; i += VEC_I16) {
            vepi16 v = vecLoad(acc + i);

            for (int a = 0; a < N_ADD; ++a) {
                v = vecAdd(v, vecLoad(adds[a] + i));
            }

            for (int s = 0; s < N_SUB; ++s) {
                v = vecSub(v, vecLoad(subs[s] + i));
            }

            vecStore(acc + i, v);
        }
    }

    void add(DefaultPiece piece, Side color, Square sq) {
        const int wi = featureIndex<WHITE>(piece, color, sq, king_sq[WHITE]);
        const int bi = featureIndex<BLACK>(piece, color, sq, king_sq[BLACK]);
        const int16_t* wa[1] = { network_weights[wi] };
        const int16_t* ba[1] = { network_weights[bi] };
        apply<1, 0>(accumulator[WHITE], wa, nullptr);
        apply<1, 0>(accumulator[BLACK], ba, nullptr);
    }

    void sub(DefaultPiece piece, Side color, Square sq) {
        const int wi = featureIndex<WHITE>(piece, color, sq, king_sq[WHITE]);
        const int bi = featureIndex<BLACK>(piece, color, sq, king_sq[BLACK]);
        const int16_t* ws[1] = { network_weights[wi] };
        const int16_t* bs[1] = { network_weights[bi] };
        apply<0, 1>(accumulator[WHITE], nullptr, ws);
        apply<0, 1>(accumulator[BLACK], nullptr, bs);
    }

    // Quiet move.
    void addSub(DefaultPiece piece, Side color, Square from_sq, Square to_sq) {
        const int wf = featureIndex<WHITE>(piece, color, from_sq, king_sq[WHITE]);
        const int bf = featureIndex<BLACK>(piece, color, from_sq, king_sq[BLACK]);
        const int wt = featureIndex<WHITE>(piece, color, to_sq, king_sq[WHITE]);
        const int bt = featureIndex<BLACK>(piece, color, to_sq, king_sq[BLACK]);
        const int16_t* wa[1] = { network_weights[wt] };
        const int16_t* ba[1] = { network_weights[bt] };
        const int16_t* ws[1] = { network_weights[wf] };
        const int16_t* bs[1] = { network_weights[bf] };
        apply<1, 1>(accumulator[WHITE], wa, ws);
        apply<1, 1>(accumulator[BLACK], ba, bs);
    }

    // Capture
    void addSubSub(DefaultPiece piece, Side color, Square from_sq, DefaultPiece captured_piece, Square to_sq) {
        const Side xcolor = static_cast<Side>(color ^ 1);
        const int wf = featureIndex<WHITE>(piece, color, from_sq, king_sq[WHITE]);
        const int bf = featureIndex<BLACK>(piece, color, from_sq, king_sq[BLACK]);
        const int wt = featureIndex<WHITE>(piece, color, to_sq, king_sq[WHITE]);
        const int bt = featureIndex<BLACK>(piece, color, to_sq, king_sq[BLACK]);
        const int wc = featureIndex<WHITE>(captured_piece, xcolor, to_sq, king_sq[WHITE]);
        const int bc = featureIndex<BLACK>(captured_piece, xcolor, to_sq, king_sq[BLACK]);
        const int16_t* wa[1] = { network_weights[wt] };
        const int16_t* ba[1] = { network_weights[bt] };
        const int16_t* ws[2] = { network_weights[wf], network_weights[wc] };
        const int16_t* bs[2] = { network_weights[bf], network_weights[bc] };
        apply<1, 2>(accumulator[WHITE], wa, ws);
        apply<1, 2>(accumulator[BLACK], ba, bs);
    }

    // Castling
    void addAddSubSub(
        DefaultPiece add1_piece, Side add1_color, Square add1_sq, DefaultPiece add2_piece, Side add2_color, Square add2_sq, DefaultPiece sub1_piece, Side sub1_color,
        Square sub1_sq, DefaultPiece sub2_piece, Side sub2_color, Square sub2_sq
    ) {
        const int wa1 = featureIndex<WHITE>(add1_piece, add1_color, add1_sq, king_sq[WHITE]);
        const int ba1 = featureIndex<BLACK>(add1_piece, add1_color, add1_sq, king_sq[BLACK]);
        const int wa2 = featureIndex<WHITE>(add2_piece, add2_color, add2_sq, king_sq[WHITE]);
        const int ba2 = featureIndex<BLACK>(add2_piece, add2_color, add2_sq, king_sq[BLACK]);
        const int ws1 = featureIndex<WHITE>(sub1_piece, sub1_color, sub1_sq, king_sq[WHITE]);
        const int bs1 = featureIndex<BLACK>(sub1_piece, sub1_color, sub1_sq, king_sq[BLACK]);
        const int ws2 = featureIndex<WHITE>(sub2_piece, sub2_color, sub2_sq, king_sq[WHITE]);
        const int bs2 = featureIndex<BLACK>(sub2_piece, sub2_color, sub2_sq, king_sq[BLACK]);
        const int16_t* wa[2] = { network_weights[wa1], network_weights[wa2] };
        const int16_t* ba[2] = { network_weights[ba1], network_weights[ba2] };
        const int16_t* ws[2] = { network_weights[ws1], network_weights[ws2] };
        const int16_t* bs[2] = { network_weights[bs1], network_weights[bs2] };
        apply<2, 2>(accumulator[WHITE], wa, ws);
        apply<2, 2>(accumulator[BLACK], ba, bs);
    }

    const int16_t* values(Side persp) const { return accumulator[persp]; }
};

inline int32_t Accumulator::evaluate(Side stm, int bucket) const {
    const Side us = stm;
    const Side them = static_cast<Side>(stm ^ 1);
    const int16_t* out_w = output_weights[bucket];

    vepi32 acc_us = vecZero32();
    vepi32 acc_them = vecZero32();
    const vepi16 lo = vecSet1(0), hi = vecSet1(L0_SCALE);
    for (size_t i = 0; i < HIDDEN_SIZE; i += VEC_I16) {
        const vepi16 cu = vecMin(vecMax(vecLoad(accumulator[us] + i), lo), hi);
        const vepi16 wu = vecLoad(out_w + i);
        acc_us = vecAdd32(acc_us, vecMAdd(vecMullo(cu, wu), cu));

        const vepi16 ct = vecMin(vecMax(vecLoad(accumulator[them] + i), lo), hi);
        const vepi16 wt = vecLoad(out_w + HIDDEN_SIZE + i);
        acc_them = vecAdd32(acc_them, vecMAdd(vecMullo(ct, wt), ct));
    }

    int64_t sum = static_cast<int64_t>(vecReduce(acc_us)) + vecReduce(acc_them);

    // Quant pipeline:
    //   sum            scale = L0^2 * L1
    //   sum / L0       scale = L0   * L1  (matches output_bias)
    //   * EVAL_SCALE / (L0 * L1) -> centipawns
    sum = sum / L0_SCALE + output_bias[bucket];
    return static_cast<int32_t>(sum * EVAL_SCALE / MUL_SCALE);
}

#endif // ACCUMULATOR_H
