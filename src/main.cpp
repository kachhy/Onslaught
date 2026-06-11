#include "datagen/datagen.h"
#include "hash/zobrist.h"
#include "movegen/attacks.h"
#include "nnue/nnue.h"
#include "search-eval/tuning.h"
#include "testing/ttbench.h"
#include "uci/uci.h"

#include <memory>

void initAttacks() {
    populateBetweenSquares();
    populateLineSquares();

    populateBishopMasks();
    populateRookMasks();

    populatePawnAttacks();
    populateKingAttacks();
    populateKnightAttacks();
    populateBishopAttacks();
    populateRookAttacks();
}

static inline void bench() {
    auto bp = std::make_unique<Board>();
    Board& b = *bp;
    auto start = std::chrono::high_resolution_clock::now();
    int best_score;
    searching = true;
    std::streambuf* original_buffer = std::cout.rdbuf(nullptr); // Silence cout
    search(b, 14, best_score, GoParams{});
    std::cout.rdbuf(original_buffer);
    searching = false;
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << nodes << " nodes " << static_cast<size_t>((static_cast<double>(nodes) / duration.count()) * 1000) << " nps" << std::endl;
}

int main(int argc, char** argv) {
    // Populate attacks
    initAttacks();

    // Populate zobrist keys
    initZobrist();

    // Populate eval data
    initEval();

#ifdef TUNING
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <Tuning dataset> <Position limit> <Epochs> <Output file> [Perturb = 0] [Threads = 1]" << std::endl;
        return 1;
    }

    std::ofstream tuned_params_out(argv[4]);
    const uint32_t dataset_size = atol(argv[2]);
    Tuner tuner(dataset_size);
    tuner.loadDataset(argv[1], dataset_size);
    const int perturb_amount = argc > 5 ? atoi(argv[5]) : 0;
    tuner.run(atol(argv[3]), argc < 7 ? 1 : atoll(argv[6]), perturb_amount);
    tuner.dumpParams(tuned_params_out);
    return 0;
#elif PERFT
    perftTests();
    return 0;
#endif
    // Load embedded network; fall back to file if not embedded or size mismatch
    const bool nnue_loaded = loadNNUEFromMemory(gNNUEWeightsData, gNNUEWeightsSize)
                             || loadNNUE(nnue_path);

    if (argc > 1 && std::string(argv[1]) == "datagen") {
        if (!nnue_loaded) {
            std::fprintf(stderr, "loadNNUE failed: no embedded net and could not load %s\n", nnue_path.c_str());
            return 1;
        }
        runDatagen();
        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "bench") {
        if (!nnue_loaded) {
            std::fprintf(stderr, "loadNNUE failed: no embedded net and could not load %s\n", nnue_path.c_str());
            return 1;
        }
        bench();
        return 0;
    }

    if (!nnue_loaded) {
        std::fprintf(stderr, "info string NNUE load failed. Falling back to HCE.\n");
        use_nnue = false;
    }
    uci();
    return 0;
}