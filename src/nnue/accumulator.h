#ifndef ACCUMULATOR_H
#define ACCUMULATOR_H

#include "core/bitboard.h"
#include "core/move.h"
#include "features.h"
#include "simd.h"
#include <cstdint>
#include <cstring>

class Board;

// Network parameters defined in nnue.cpp
extern int16_t network_weights[INPUT_SIZE * NUM_KING_BUCKETS][HIDDEN_SIZE];
extern int16_t network_biases[HIDDEN_SIZE];
extern int16_t output_weights[NUM_OUTPUT_BUCKETS][2 * HIDDEN_SIZE];
extern int16_t output_bias[NUM_OUTPUT_BUCKETS];
extern int16_t complexity_weights[NUM_OUTPUT_BUCKETS][2 * HIDDEN_SIZE];
extern int16_t complexity_bias[NUM_OUTPUT_BUCKETS];

class alignas(64) Accumulator {
private:
    int16_t accumulator[2][HIDDEN_SIZE]; // [perspective]
public:
    uint8_t king_sq[2]; // [side]
    bool accumulator_dirty = false;
    Move move; // Move whose delta is owed when accumulator_dirty
    Side stm;
    Piece captured;

    Accumulator() = default;

    void record(Move m, Side s, Piece cap, uint8_t wk, uint8_t bk) {
        move = m; stm = s; captured = cap;
        king_sq[WHITE] = wk;
        king_sq[BLACK] = bk;
        accumulator_dirty = true;
    }

    void seedFrom(const Accumulator& prev) {
        memcpy(accumulator, prev.accumulator, sizeof(accumulator));
    }

    void refreshIfKingCrossed(const Board& board, Square from, Square to) {
        if (kingBucket(from, stm) != kingBucket(to, stm) || kingNeedsMirror(from) != kingNeedsMirror(to)) {
            refresh(board);
        }
    }

    void applyDelta(const Accumulator& prev) {
        accumulator_dirty = false;
        if (move == NO_MOVE) {
            seedFrom(prev); // null move, so the position is unchanged
            return;
        }

        const DefaultPiece moved_dp = makeDefaultPiece(MovePiece(move));
        const Side us = stm;
        const Side them = static_cast<Side>(us ^ 1);
        const Square from = From(move);
        const Square to = To(move);

        if (moved_dp == KING) { // same bucket + mirror only
            king_sq[us] = static_cast<uint8_t>(to);
        }

        if (Castle(move)) {
            Square rook_from = NO_SQUARE, rook_to = NO_SQUARE;
            switch (to) {
            case G1:
                rook_from = H1;
                rook_to = F1;
                break;
            case C1:
                rook_from = A1;
                rook_to = D1;
                break;
            case G8:
                rook_from = H8;
                rook_to = F8;
                break;
            case C8:
                rook_from = A8;
                rook_to = D8;
                break;
            default: seedFrom(prev); return;
            }

            addAddSubSub(prev, KING, us, to, ROOK, us, rook_to, KING, us, from, ROOK, us, rook_from);
            return;
        }

        if (Prom(move)) {
            sub(prev, PAWN, us, from);
            add(*this, promPiece(move), us, to);
            if (Capture(move)) {
                sub(*this, makeDefaultPiece(captured), them, to);
            }

            return;
        }

        if (IsEP(move)) {
            const Square cap_sq = static_cast<Square>(static_cast<int>(to) - (us == WHITE ? NORTH : SOUTH));
            sub(prev, PAWN, us, from);
            add(*this, PAWN, us, to);
            sub(*this, PAWN, them, cap_sq);
            return;
        }

        if (Capture(move)) {
            addSubSub(prev, moved_dp, us, from, makeDefaultPiece(captured), to);
            return;
        }

        addSub(prev, moved_dp, us, from, to);
        return;
    }

    void reset() {
        for (size_t i = 0; i < HIDDEN_SIZE; i++) {
            accumulator[WHITE][i] = network_biases[i];
            accumulator[BLACK][i] = network_biases[i];
        }
        accumulator_dirty = false;
    }

    int32_t evaluate(Side stm, int bucket) const;
    int32_t complexity(Side stm, int bucket) const;
    int64_t forward(Side stm, const int16_t* out_w) const;

    // Defined in nnue.cpp to break a cycle.
    void refresh(const Board& board);

    template <int N_ADD, int N_SUB>
    inline void apply(int16_t* dst, const int16_t* src, const int16_t* const* adds, const int16_t* const* subs) {
        for (size_t i = 0; i < HIDDEN_SIZE; i += VEC_I16) {
            vepi16 v = vecLoad(src + i);

            for (int a = 0; a < N_ADD; ++a) {
                v = vecAdd(v, vecLoad(adds[a] + i));
            }

            for (int s = 0; s < N_SUB; ++s) {
                v = vecSub(v, vecLoad(subs[s] + i));
            }

            vecStore(dst + i, v);
        }
    }

    void add(const Accumulator& src, DefaultPiece piece, Side color, Square sq) {
        const int wi = featureIndex<WHITE>(piece, color, sq, king_sq[WHITE]);
        const int bi = featureIndex<BLACK>(piece, color, sq, king_sq[BLACK]);
        const int16_t* wa[1] = { network_weights[wi] };
        const int16_t* ba[1] = { network_weights[bi] };
        apply<1, 0>(accumulator[WHITE], src.accumulator[WHITE], wa, nullptr);
        apply<1, 0>(accumulator[BLACK], src.accumulator[BLACK], ba, nullptr);
    }

    void sub(const Accumulator& src, DefaultPiece piece, Side color, Square sq) {
        const int wi = featureIndex<WHITE>(piece, color, sq, king_sq[WHITE]);
        const int bi = featureIndex<BLACK>(piece, color, sq, king_sq[BLACK]);
        const int16_t* ws[1] = { network_weights[wi] };
        const int16_t* bs[1] = { network_weights[bi] };
        apply<0, 1>(accumulator[WHITE], src.accumulator[WHITE], nullptr, ws);
        apply<0, 1>(accumulator[BLACK], src.accumulator[BLACK], nullptr, bs);
    }

    // Quiet move.
    void addSub(const Accumulator& src, DefaultPiece piece, Side color, Square from_sq, Square to_sq) {
        const int wf = featureIndex<WHITE>(piece, color, from_sq, king_sq[WHITE]);
        const int bf = featureIndex<BLACK>(piece, color, from_sq, king_sq[BLACK]);
        const int wt = featureIndex<WHITE>(piece, color, to_sq, king_sq[WHITE]);
        const int bt = featureIndex<BLACK>(piece, color, to_sq, king_sq[BLACK]);
        const int16_t* wa[1] = { network_weights[wt] };
        const int16_t* ba[1] = { network_weights[bt] };
        const int16_t* ws[1] = { network_weights[wf] };
        const int16_t* bs[1] = { network_weights[bf] };
        apply<1, 1>(accumulator[WHITE], src.accumulator[WHITE], wa, ws);
        apply<1, 1>(accumulator[BLACK], src.accumulator[BLACK], ba, bs);
    }

    // Capture
    void addSubSub(const Accumulator& src, DefaultPiece piece, Side color, Square from_sq, DefaultPiece captured_piece, Square to_sq) {
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
        apply<1, 2>(accumulator[WHITE], src.accumulator[WHITE], wa, ws);
        apply<1, 2>(accumulator[BLACK], src.accumulator[BLACK], ba, bs);
    }

    // Castling
    void addAddSubSub(
        const Accumulator& src, DefaultPiece add1_piece, Side add1_color, Square add1_sq, DefaultPiece add2_piece, Side add2_color, Square add2_sq, DefaultPiece sub1_piece,
        Side sub1_color, Square sub1_sq, DefaultPiece sub2_piece, Side sub2_color, Square sub2_sq
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
        apply<2, 2>(accumulator[WHITE], src.accumulator[WHITE], wa, ws);
        apply<2, 2>(accumulator[BLACK], src.accumulator[BLACK], ba, bs);
    }

    const int16_t* values(Side persp) const { return accumulator[persp]; }
};

// Squared-clipped-ReLU forward pass through one output head's weights
inline int64_t Accumulator::forward(Side stm, const int16_t* out_w) const {
    const Side us = stm;
    const Side them = static_cast<Side>(stm ^ 1);

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

    return static_cast<int64_t>(vecReduce(acc_us)) + vecReduce(acc_them);
}

inline int32_t Accumulator::evaluate(Side stm, int bucket) const {
    // Quant pipeline:
    //   sum            scale = L0^2 * L1
    //   sum / L0       scale = L0   * L1  (matches output_bias)
    //   * EVAL_SCALE / (L0 * L1) -> centipawns
    int64_t sum = forward(stm, output_weights[bucket]) / L0_SCALE + output_bias[bucket];
    return static_cast<int32_t>(sum * EVAL_SCALE / MUL_SCALE);
}

inline int32_t Accumulator::complexity(Side stm, int bucket) const {
    // Same quant pipeline as evaluate(), using the complexity head's weights/bias.
    int64_t sum = forward(stm, complexity_weights[bucket]) / L0_SCALE + complexity_bias[bucket];
    return static_cast<int32_t>(sum * EVAL_SCALE / MUL_SCALE);
}

#endif // ACCUMULATOR_H
