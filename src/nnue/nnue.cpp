#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin.h"

#ifdef EVALFILE
INCBIN(NNUEWeights, EVALFILE);
#else
// No embedded network - falls back to file loading at runtime
extern const unsigned char gNNUEWeightsData[] = {};
extern const unsigned int gNNUEWeightsSize = 0;
#endif

#include "nnue.h"

#include "board/board.h"
#include "core/bitboard.h"
#include "core/move.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>

// Global game board, declared in uci.cpp
extern Board board;

std::string nnue_path = "nn-472028ca947a1ad5-v2cn.nnue"; // Default NNUE

// Network storage
alignas(64) int16_t network_weights[INPUT_SIZE * NUM_KING_BUCKETS][HIDDEN_SIZE] = {};
int16_t network_biases[HIDDEN_SIZE] = {};
alignas(64) int16_t output_weights[NUM_OUTPUT_BUCKETS][2 * HIDDEN_SIZE] = {};
int16_t output_bias[NUM_OUTPUT_BUCKETS] = {};
alignas(64) int16_t complexity_l2a_weights[CPLX_L1][2 * HIDDEN_SIZE] = {};
int16_t complexity_l2a_bias[CPLX_L1] = {};
alignas(64) int16_t complexity_l2b_weights[NUM_OUTPUT_BUCKETS][CPLX_L1] = {};
int16_t complexity_l2b_bias[NUM_OUTPUT_BUCKETS] = {};

void Accumulator::refresh(const Board& board) {
    reset();
    king_sq[WHITE] = static_cast<uint8_t>(getLSB(board.getPieceBB(WHITE_KING)));
    king_sq[BLACK] = static_cast<uint8_t>(getLSB(board.getPieceBB(BLACK_KING)));
    for (uint8_t sq = 0; sq < 64; sq++) {
        const Piece pc = board.pieceAt(sq);
        if (pc == NO_PIECE) {
            continue;
        }

        add(*this, makeDefaultPiece(pc), getPieceSide(pc), static_cast<Square>(sq));
    }

    accumulator_dirty = false;
}

int evaluate(const Board& board) {
    board.accumulatorPropagate(); // Bring the top accumulator up to date along the lazy chain
    Accumulator& accum = board.getAccumulator();
    const uint8_t piece_count = bitCount(board.getOcc(BOTH));
    return (accum.evaluate(board.getSTM(), outputBucket(piece_count)) * NNUE_WEIGHT_SCALAR / 100);
}

int complexity(const Board& board) {
    board.accumulatorPropagate(); // Bring the top accumulator up to date along the lazy chain
    Accumulator& accum = board.getAccumulator();
    const uint8_t piece_count = bitCount(board.getOcc(BOTH));
    return (accum.complexity(board.getSTM(), outputBucket(piece_count)) * NNUE_WEIGHT_SCALAR / 100);
}

double complexityPercent(const Board& board) { return 1.0 / (1.0 + std::exp(-static_cast<double>(complexity(board)) / EVAL_SCALE)); }

namespace {
constexpr size_t CPLX_L1 = 32;

constexpr size_t FT_WEIGHT_COUNT = INPUT_SIZE * HIDDEN_SIZE * NUM_KING_BUCKETS;
constexpr size_t FT_BIAS_COUNT = HIDDEN_SIZE;
constexpr size_t OUT_WEIGHT_COUNT = NUM_OUTPUT_BUCKETS * 2 * HIDDEN_SIZE;
constexpr size_t OUT_BIAS_COUNT = NUM_OUTPUT_BUCKETS;

// New 2-layer complexity architecture counts
constexpr size_t CPX_L2A_WEIGHT_COUNT = CPLX_L1 * 2 * HIDDEN_SIZE;
constexpr size_t CPX_L2A_BIAS_COUNT = CPLX_L1;
constexpr size_t CPX_L2B_WEIGHT_COUNT = NUM_OUTPUT_BUCKETS * CPLX_L1;
constexpr size_t CPX_L2B_BIAS_COUNT = NUM_OUTPUT_BUCKETS;

constexpr size_t EXPECTED_BYTES =
    (FT_WEIGHT_COUNT + FT_BIAS_COUNT + OUT_WEIGHT_COUNT + OUT_BIAS_COUNT + CPX_L2A_WEIGHT_COUNT + CPX_L2A_BIAS_COUNT + CPX_L2B_WEIGHT_COUNT + CPX_L2B_BIAS_COUNT) *
    sizeof(std::int16_t);

// Bullet pads the dumped Parameters struct to a 64-byte boundary, so we accept either the exact count OR to the next 64 bytes
constexpr size_t ALIGN_BYTES = 64;
constexpr size_t EXPECTED_BYTES_PADDED = (EXPECTED_BYTES + ALIGN_BYTES - 1) / ALIGN_BYTES * ALIGN_BYTES;

bool readExact(std::ifstream& in, void* dst, std::size_t bytes) {
    in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(bytes));
    return in.gcount() == static_cast<std::streamsize>(bytes);
}
} // namespace

bool loadNNUEFromMemory(const unsigned char* data, size_t size) {
    if (!data || size == 0) {
        return false;
    }
    if (size != EXPECTED_BYTES && size != EXPECTED_BYTES_PADDED) {
        return false;
    }

    const char* ptr = reinterpret_cast<const char*>(data);
    std::memcpy(network_weights, ptr, FT_WEIGHT_COUNT * sizeof(int16_t));
    ptr += FT_WEIGHT_COUNT * sizeof(int16_t);
    std::memcpy(network_biases, ptr, FT_BIAS_COUNT * sizeof(int16_t));
    ptr += FT_BIAS_COUNT * sizeof(int16_t);
    std::memcpy(output_weights, ptr, OUT_WEIGHT_COUNT * sizeof(int16_t));
    ptr += OUT_WEIGHT_COUNT * sizeof(int16_t);
    std::memcpy(output_bias, ptr, OUT_BIAS_COUNT * sizeof(int16_t));
    ptr += OUT_BIAS_COUNT * sizeof(int16_t);
    std::memcpy(complexity_l2a_weights, ptr, CPX_L2A_WEIGHT_COUNT * sizeof(int16_t));
    ptr += CPX_L2A_WEIGHT_COUNT * sizeof(int16_t);
    std::memcpy(complexity_l2a_bias, ptr, CPX_L2A_BIAS_COUNT * sizeof(int16_t));
    ptr += CPX_L2A_BIAS_COUNT * sizeof(int16_t);
    std::memcpy(complexity_l2b_weights, ptr, CPX_L2B_WEIGHT_COUNT * sizeof(int16_t));
    ptr += CPX_L2B_WEIGHT_COUNT * sizeof(int16_t);
    std::memcpy(complexity_l2b_bias, ptr, CPX_L2B_BIAS_COUNT * sizeof(int16_t));

    board.refreshAccumulator();

    return true;
}

bool loadNNUE(const std::filesystem::path& path) {
    std::error_code ec;
    const auto file_size = std::filesystem::file_size(path, ec);

    if (ec) {
        std::fprintf(stderr, "loadNNUE: cannot stat %s (%s)\n", path.string().c_str(), ec.message().c_str());
        return false;
    }

    if (file_size != EXPECTED_BYTES && file_size != EXPECTED_BYTES_PADDED) {
        std::fprintf(
            stderr, "loadNNUE: size mismatch for %s - got %llu bytes, expected %zu (or %zu padded; INPUT_SIZE=%zu HIDDEN_SIZE=%zu int16)\n", path.string().c_str(),
            static_cast<unsigned long long>(file_size), EXPECTED_BYTES, EXPECTED_BYTES_PADDED, static_cast<size_t>(INPUT_SIZE), static_cast<size_t>(HIDDEN_SIZE)
        );
        if (file_size > 2 && (file_size - 2) % 1542 == 0) {
            std::fprintf(stderr, "  Implied HIDDEN_SIZE = %llu (int16 bullet format)\n", static_cast<unsigned long long>((file_size - 2) / 1542));
        }

        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    in.exceptions(std::ios::badbit);

    if (!readExact(in, network_weights, FT_WEIGHT_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, network_biases, FT_BIAS_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, output_weights, OUT_WEIGHT_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, output_bias, OUT_BIAS_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, complexity_l2a_weights, CPX_L2A_WEIGHT_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, complexity_l2a_bias, CPX_L2A_BIAS_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, complexity_l2b_weights, CPX_L2B_WEIGHT_COUNT * sizeof(std::int16_t))) {
        return false;
    }
    if (!readExact(in, complexity_l2b_bias, CPX_L2B_BIAS_COUNT * sizeof(std::int16_t))) {
        return false;
    }

    board.refreshAccumulator();
    return true;
}
