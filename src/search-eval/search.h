#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"
#include "search-eval/spsa.h"
#include <chrono>
#include <cstddef>

// Search terms live in spsa.h
constexpr int LMR_TABLE_SIZE = 64;
constexpr int LMR_GRAIN = 1024; // LMR reductions and their adjustments are in 1/LMR_GRAIN ply
extern int LMR_TABLE[LMR_TABLE_SIZE][LMR_TABLE_SIZE];
void initLMR();

extern thread_local uint64_t nodes;
extern thread_local uint64_t tb_hits;
extern int multi_pv;
extern short syz_probe_depth;
extern short syz_probe_limit;
extern bool syz_fmr;
extern bool syz_dtz;

struct GoParams;

Move search(Board& board, int max_depth, int& best_score, const GoParams& params);

#endif // SEARCH_H