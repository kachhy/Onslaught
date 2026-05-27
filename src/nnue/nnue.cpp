#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin.h"

#ifdef EVALFILE
INCBIN(NNUEWeights, EVALFILE);
#else
// No embedded network — falls back to file loading at runtime
extern const unsigned char gNNUEWeightsData[] = {};
extern const unsigned int  gNNUEWeightsSize   = 0;
#endif

#include "nnue.h"

#include "board/board.h"
#include "core/bitboard.h"
#include "core/move.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>

// Global game board, declared in uci.cpp
extern Board board;

std::string nnue_path = "nn-0a63fbab92d2bb57-64.nnue"; // Default NNUE

// Network storage
int16_t network_weights[INPUT_SIZE][HIDDEN_SIZE] = {};
int16_t network_biases[HIDDEN_SIZE] = {};
int16_t output_weights[2 * HIDDEN_SIZE] = {};
int16_t output_bias = 0;

void Accumulator::refresh(const Board& board) {
    reset();
    for (uint8_t sq = 0; sq < 64; sq++) {
        const Piece pc = board.pieceAt(sq);
        if (pc == NO_PIECE) {
            continue;
        }

        add(makeDefaultPiece(pc), getPieceSide(pc), static_cast<Square>(sq));
    }
}

void Accumulator::onMove(Move move, const Board& board) {
    const Piece moved_piece = MovePiece(move);
    const DefaultPiece moved_dp = makeDefaultPiece(moved_piece);
    const Side us = board.getSTM();
    const Side them = static_cast<Side>(us ^ 1);
    const Square from = From(move);
    const Square to = To(move);

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
        default: return;
        }

        addAddSubSub(KING, us, to, ROOK, us, rook_to, KING, us, from, ROOK, us, rook_from);
        return;
    }

    if (Prom(move)) {
        sub(PAWN, us, from);
        add(promPiece(move), us, to);
        if (Capture(move)) {
            sub(makeDefaultPiece(board.pieceAt(to)), them, to);
        }

        return;
    }

    if (IsEP(move)) {
        const Square cap_sq = static_cast<Square>(static_cast<int>(to) - (us == WHITE ? NORTH : SOUTH));
        sub(PAWN, us, from);
        add(PAWN, us, to);
        sub(PAWN, them, cap_sq);
        return;
    }

    if (Capture(move)) {
        addSubSub(moved_dp, us, from, makeDefaultPiece(board.pieceAt(to)), to);
        return;
    }

    addSub(moved_dp, us, from, to);
}

int evaluate(const Accumulator& accum, const Side stm) { return accum.evaluate(stm) * NNUE_WEIGHT_SCALAR / 100; }

namespace {
constexpr size_t FT_WEIGHT_COUNT = INPUT_SIZE * HIDDEN_SIZE;
constexpr size_t FT_BIAS_COUNT = HIDDEN_SIZE;
constexpr size_t OUT_WEIGHT_COUNT = 2 * HIDDEN_SIZE;
constexpr size_t OUT_BIAS_COUNT = 1;
constexpr size_t EXPECTED_BYTES = (FT_WEIGHT_COUNT + FT_BIAS_COUNT + OUT_WEIGHT_COUNT + OUT_BIAS_COUNT) * sizeof(std::int16_t);

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
        std::fprintf(stderr, "loadNNUE: embedded size mismatch (%zu bytes, expected %zu or %zu padded)\n",
                     size, EXPECTED_BYTES, EXPECTED_BYTES_PADDED);
        return false;
    }

    const char* ptr = reinterpret_cast<const char*>(data);
    std::memcpy(network_weights, ptr, FT_WEIGHT_COUNT * sizeof(int16_t));
    ptr += FT_WEIGHT_COUNT * sizeof(int16_t);
    std::memcpy(network_biases, ptr, FT_BIAS_COUNT * sizeof(int16_t));
    ptr += FT_BIAS_COUNT * sizeof(int16_t);
    std::memcpy(output_weights, ptr, OUT_WEIGHT_COUNT * sizeof(int16_t));
    ptr += OUT_WEIGHT_COUNT * sizeof(int16_t);
    std::memcpy(&output_bias, ptr, OUT_BIAS_COUNT * sizeof(int16_t));
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
    if (!readExact(in, &output_bias, OUT_BIAS_COUNT * sizeof(std::int16_t))) {
        return false;
    }

    board.refreshAccumulator();
    return true;
}
