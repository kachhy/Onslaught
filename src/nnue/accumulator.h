#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include "core/bitboard.h"
#include "core/move.h"
#include "features.h"
#include <cstdint>

class Board;

// Network parameters defined in nnue.cpp
extern int16_t network_weights[INPUT_SIZE][HIDDEN_SIZE];
extern int16_t network_biases[HIDDEN_SIZE];
extern int16_t output_weights[2 * HIDDEN_SIZE]; // [0..H-1]=us, [H..2H-1]=them
extern int16_t output_bias;

class alignas(64) Accumulator {
private:
    int16_t accumulator[2][HIDDEN_SIZE]; // [perspective]

public:
    Accumulator() = default;

    void reset() {
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] = network_biases[i];
            accumulator[BLACK][i] = network_biases[i];
        }
    }

    int32_t evaluate(Side stm) const;

    // Defined in nnue.cpp to break a cycle
    void refresh(const Board& board);
    void onMove(Move move, const Board& board);

    void add(DefaultPiece piece, Side color, Square sq) {
        const int wi = featureIndex<WHITE>(piece, color, sq);
        const int bi = featureIndex<BLACK>(piece, color, sq);
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] += network_weights[wi][i];
            accumulator[BLACK][i] += network_weights[bi][i];
        }
    }

    void sub(DefaultPiece piece, Side color, Square sq) {
        const int wi = featureIndex<WHITE>(piece, color, sq);
        const int bi = featureIndex<BLACK>(piece, color, sq);
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] -= network_weights[wi][i];
            accumulator[BLACK][i] -= network_weights[bi][i];
        }
    }

    // Quiet move.
    void addSub(DefaultPiece piece, Side color, Square from_sq, Square to_sq) {
        const int wf = featureIndex<WHITE>(piece, color, from_sq);
        const int bf = featureIndex<BLACK>(piece, color, from_sq);
        const int wt = featureIndex<WHITE>(piece, color, to_sq);
        const int bt = featureIndex<BLACK>(piece, color, to_sq);
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] += network_weights[wt][i] - network_weights[wf][i];
            accumulator[BLACK][i] += network_weights[bt][i] - network_weights[bf][i];
        }
    }

    // Capture
    void addSubSub(DefaultPiece piece, Side color, Square from_sq, DefaultPiece captured_piece, Square to_sq) {
        const Side xcolor = static_cast<Side>(color ^ 1);
        const int wf = featureIndex<WHITE>(piece, color, from_sq);
        const int bf = featureIndex<BLACK>(piece, color, from_sq);
        const int wt = featureIndex<WHITE>(piece, color, to_sq);
        const int bt = featureIndex<BLACK>(piece, color, to_sq);
        const int wc = featureIndex<WHITE>(captured_piece, xcolor, to_sq);
        const int bc = featureIndex<BLACK>(captured_piece, xcolor, to_sq);
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] += network_weights[wt][i] - network_weights[wf][i] - network_weights[wc][i];
            accumulator[BLACK][i] += network_weights[bt][i] - network_weights[bf][i] - network_weights[bc][i];
        }
    }

    // Castling
    void addAddSubSub(
        DefaultPiece add1_piece, Side add1_color, Square add1_sq, DefaultPiece add2_piece, Side add2_color, Square add2_sq, DefaultPiece sub1_piece, Side sub1_color,
        Square sub1_sq, DefaultPiece sub2_piece, Side sub2_color, Square sub2_sq
    ) {
        const int wa1 = featureIndex<WHITE>(add1_piece, add1_color, add1_sq);
        const int ba1 = featureIndex<BLACK>(add1_piece, add1_color, add1_sq);
        const int wa2 = featureIndex<WHITE>(add2_piece, add2_color, add2_sq);
        const int ba2 = featureIndex<BLACK>(add2_piece, add2_color, add2_sq);
        const int ws1 = featureIndex<WHITE>(sub1_piece, sub1_color, sub1_sq);
        const int bs1 = featureIndex<BLACK>(sub1_piece, sub1_color, sub1_sq);
        const int ws2 = featureIndex<WHITE>(sub2_piece, sub2_color, sub2_sq);
        const int bs2 = featureIndex<BLACK>(sub2_piece, sub2_color, sub2_sq);
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] += network_weights[wa1][i] + network_weights[wa2][i] - network_weights[ws1][i] - network_weights[ws2][i];
            accumulator[BLACK][i] += network_weights[ba1][i] + network_weights[ba2][i] - network_weights[bs1][i] - network_weights[bs2][i];
        }
    }

    const int16_t* values(Side persp) const { return accumulator[persp]; }
};

inline int32_t Accumulator::evaluate(Side stm) const {
    const Side us = stm;
    const Side them = static_cast<Side>(stm ^ 1);

    int64_t sum = 0;
    for (size_t i = 0; i < HIDDEN_SIZE; i++) {
        sum += static_cast<int64_t>(screlu(accumulator[us][i])) * output_weights[i];
        sum += static_cast<int64_t>(screlu(accumulator[them][i])) * output_weights[HIDDEN_SIZE + i];
    }
    
    // Quant pipeline:
    //   sum            scale = L0^2 * L1
    //   sum / L0       scale = L0   * L1  (matches output_bias)
    //   * EVAL_SCALE / (L0 * L1) -> centipawns
    sum = sum / L0_SCALE + output_bias;
    return static_cast<int32_t>(sum * EVAL_SCALE / MUL_SCALE);
}

#endif // ACCUMULATOR_H
