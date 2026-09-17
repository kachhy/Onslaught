#include "board/board.h"
#include "nnue/nnue.h"
#include <cstdio>
#include <vector>

extern Board board;

namespace {
constexpr const char* positions[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",
    "8/2k5/8/3p4/4P3/8/5K2/8 w - - 0 1",
    "8/5k2/8/4p3/3P4/8/2K5/8 w - - 0 1",
};

bool matchesFullRefresh(const char* stage) {
    Accumulator expected;
    expected.king_sq[WHITE] = getLSB(board.getPieceBB(WHITE_KING));
    expected.king_sq[BLACK] = getLSB(board.getPieceBB(BLACK_KING));
    expected.reset();
    for (uint8_t sq = 0; sq < 64; ++sq) {
        const Piece piece = board.pieceAt(sq);
        if (piece != NO_PIECE) {
            expected.add(expected, makeDefaultPiece(piece), getPieceSide(piece), static_cast<Square>(sq));
        }
    }

    for (Side side : { WHITE, BLACK }) {
        for (size_t i = 0; i < HIDDEN_SIZE; ++i) {
            if (board.getAccumulator().values(side)[i] != expected.values(side)[i]) {
                std::fprintf(stderr, "%s: accumulator mismatch (side %d, neuron %zu)\n", stage, side, i);
                return false;
            }
        }
    }
    return true;
}

bool checkPositions(const char* stage) {
    // Check the loader's immediate refresh as well as warm entries for other kings.
    if (!matchesFullRefresh(stage)) {
        return false;
    }
    for (const char* fen : positions) {
        if (!board.loadFEN(fen) || !matchesFullRefresh(stage)) {
            return false;
        }
    }
    return true;
}
} // namespace

int main() {
    if (gNNUEWeightsSize == 0) {
        std::fprintf(stderr, "This test requires an embedded network.\n");
        return 1;
    }

    // Force pre-load cache entries regardless of translation-unit initialization order.
    for (const char* fen : positions) {
        if (!board.loadFEN(fen)) {
            return 1;
        }
    }
    if (!loadNNUEFromMemory(gNNUEWeightsData, gNNUEWeightsSize) || !checkPositions("initial memory load")) {
        return 1;
    }

    // Replace the network while several king-square entries are warm. A zero net
    // also changes biases, which cannot be corrected by piece deltas alone.
    const std::vector<unsigned char> zeroNetwork(gNNUEWeightsSize, 0);
    if (!loadNNUEFromMemory(zeroNetwork.data(), zeroNetwork.size()) || !checkPositions("replacement memory load")) {
        return 1;
    }
    if (!loadNNUE(nnue_path) || !checkPositions("file load")) {
        return 1;
    }
    if (!loadNNUEFromMemory(zeroNetwork.data(), zeroNetwork.size()) || !checkPositions("second replacement")) {
        return 1;
    }
    if (!loadNNUEFromMemory(gNNUEWeightsData, gNNUEWeightsSize) || !checkPositions("restored embedded network")) {
        return 1;
    }
    std::puts("NNUE cache load tests passed");
    return 0;
}
