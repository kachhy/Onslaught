#include "ttbench.h"
#include "uci/uci.h"
#include <chrono>

// int wtime = -1, btime = -1;
// int winc = 0, binc = 0;
// int movestogo = 0; // 0 means unknown (sudden death)
// int depth = -1;
// long long nodes = -1;
// int movetime = -1;
// bool infinite = false;
// bool ponder = false;

void benchTT() {
    Board b;
    int best_score;
    auto start = std::chrono::high_resolution_clock::now();
    searching = true;
    search(b, 18, best_score, GoParams{});
    searching = false;
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << "TTBench -- Nodes: " << nodes << " NPS: " << (static_cast<double>(nodes) / duration.count()) / static_cast<double>(1000) << " MNPS" << std::endl;
}