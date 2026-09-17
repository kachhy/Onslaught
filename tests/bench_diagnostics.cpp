// Reuse startup and the actual benchmark without adding diagnostics to the UCI interface.
#define main engineMain
#include "../src/main.cpp"
#undef main

#include "hash/transposition.h"
#include <cstdio>

namespace {
uint64_t fingerprint(const unsigned char* data, size_t size) {
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash = (hash ^ data[i]) * 1099511628211ULL;
    }
    return hash;
}

bool verifyAccumulator(const Board& position) {
    Accumulator expected;
    expected.king_sq[WHITE] = getLSB(position.getPieceBB(WHITE_KING));
    expected.king_sq[BLACK] = getLSB(position.getPieceBB(BLACK_KING));
    expected.reset();
    for (uint8_t sq = 0; sq < 64; ++sq) {
        Piece piece = position.pieceAt(sq);
        if (piece != NO_PIECE) {
            expected.add(expected, makeDefaultPiece(piece), getPieceSide(piece), static_cast<Square>(sq));
        }
    }
    position.accumulatorPropagate();
    for (Side side : { WHITE, BLACK }) {
        if (std::memcmp(expected.values(side), position.getAccumulator().values(side), HIDDEN_SIZE * sizeof(int16_t))) {
            return false;
        }
    }
    return true;
}
} // namespace

int main() {
    std::printf("compiler: %s\n", __VERSION__);
    std::printf("SIMD lanes: %d; Board: %zu; TT bucket: %zu; TT capacity: %zu\n",
                VEC_I16, sizeof(Board), sizeof(EntryTriple), tt.capacity());
    std::printf("embedded network: %u bytes; FNV-1a: %016llx\n", gNNUEWeightsSize,
                static_cast<unsigned long long>(fingerprint(gNNUEWeightsData, gNNUEWeightsSize)));

    initAttacks();
    initZobrist();
    initEval();
    initLMR();
    if (!loadNNUEFromMemory(gNNUEWeightsData, gNNUEWeightsSize) && !loadNNUE(nnue_path)) {
        return 1;
    }
    std::printf("LMR FNV-1a: %016llx; weights FNV-1a: %016llx\n",
                static_cast<unsigned long long>(fingerprint(reinterpret_cast<const unsigned char*>(LMR_TABLE), sizeof(LMR_TABLE))),
                static_cast<unsigned long long>(fingerprint(reinterpret_cast<const unsigned char*>(network_weights), sizeof(network_weights))));

    auto position = std::make_unique<Board>();
    if (!verifyAccumulator(*position)) {
        std::fprintf(stderr, "Start-position accumulator differs from full rebuild\n");
        return 1;
    }
    std::printf("start hash: %016llx; evaluation: %d\n",
                static_cast<unsigned long long>(position->hash()), eval(*position));
    for (Side side : { WHITE, BLACK }) {
        std::printf("accumulator %d FNV-1a: %016llx\n", side,
                    static_cast<unsigned long long>(fingerprint(reinterpret_cast<const unsigned char*>(position->getAccumulator().values(side)), HIDDEN_SIZE * sizeof(int16_t))));
    }

    // Match bench's search parameters and print the normally suppressed depth trace.
    // Disable stdin polling so an EOF or queued terminal input cannot affect the run.
    stdin_enabled = false;
    searching = true;
    int best_score;
    search(*position, 14, best_score, GoParams{});
    searching = false;
    std::printf("diagnostic bench: %llu nodes; score: %d\n", static_cast<unsigned long long>(nodes), best_score);
    return 0;
}
