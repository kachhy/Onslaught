#include "hash/zobrist.h"
#include "movegen/attacks.h"
#include "nnue/nnue.h"
#include "search-eval/tuning.h"
#include "testing/perft.h"
#include "testing/ttbench.h"
#include "uci/uci.h"

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

int main(int argc, char** argv) {
    // Populate attacks
    initAttacks();

    // Populate zobrist keys
    initZobrist();

    // Populate eval data
    initEval();

    // Run TTBench
    // benchTT();

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
    if (argc > 1 && std::string(argv[1]) == "bench") {
        bench();
    } else {
        if (!loadNNUE("nn_Q16_gen1.bin")) {
            std::fprintf(stderr, "loadNNUE failed for nn_Q16_gen1.bin\n");
            return 1;
        }
        
        uci();
    }
    return 0;
}