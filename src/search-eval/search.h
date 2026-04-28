#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"
#include <chrono>
#include <cstddef>

constexpr uint8_t RFP_MARGIN = 75;
constexpr uint8_t ASPIRATION_MARGIN = 12;
constexpr uint16_t FUTILITY_MARGIN = 150;
constexpr uint16_t RAZOR_MARGIN = 350;
constexpr float LMR_VALUE = 1;
constexpr float LMR_SCALAR = 2;
constexpr int MAX_HISTORY = 16384;
constexpr uint8_t LMP_BASE = 3;

struct GoParams;

void initSearch();
int search(Board& board, int depth, int alpha, int beta, size_t hard_cap, long long max_nodes, std::chrono::high_resolution_clock::time_point start, int ply = 0);
Move search(Board& board, int max_depth, int& best_score, const GoParams& params);

#endif // SEARCH_H