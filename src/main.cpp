#include "hash/zobrist.h"
#include "movegen/attacks.h"
#include "search-eval/tuning.h"
#include "testing/perft.h"
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

#ifdef TUNING
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <Tuning dataset> <Position limit> <Epochs> <Output file> [Threads = 1]" << std::endl;
        return 1;
    }

    std::ofstream tuned_params_out(argv[4]);
    const uint32_t dataset_size = atol(argv[2]);
    Tuner tuner(dataset_size);
    tuner.loadDataset(argv[1], dataset_size);
    tuner.run(atol(argv[3]), argc < 6 ? 1 : atoll(argv[5]));
    tuner.dumpParams(tuned_params_out);
#elif PERFT
    perftTests();
    return 0;
#endif
    if (argc > 1 && std::string(argv[1]) == "bench") {
        bench();
    } else {
        uci();
    }
    return 0;
}