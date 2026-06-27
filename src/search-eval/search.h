#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"
#include <chrono>
#include <cstddef>

constexpr uint8_t RFP_MARGIN = 46;
constexpr uint8_t ASPIRATION_MARGIN = 35;
constexpr uint16_t FUTILITY_MARGIN = 108;
constexpr uint16_t RAZOR_MARGIN = 585;
constexpr float LMR_VALUE = 1.470263;
constexpr float LMR_SCALAR = 2.523939;
constexpr int HIST_LMR_DIVISOR = 8632; // history score -> LMR reduction adjustment
constexpr uint8_t NMP_DEPTH_CUTOFF = 5;
constexpr uint8_t RAZORING_DEPTH_MAX = 2;
constexpr uint8_t IIR_DEPTH_CUTOFF = 3;
constexpr uint8_t FP_DEPTH_MAX = 5;
constexpr uint8_t LMP_DEPTH_MAX = 4;
constexpr int LMP_BASE = 9;
constexpr uint8_t LMR_MOVES_CUTOFF = 8;
constexpr uint8_t LMR_DEPTH_CUTOFF = 2;
constexpr uint8_t ASPIRATION_DEPTH_CUTOFF = 4;
constexpr float ASPIRATION_SCALAR = 1.290771;
constexpr int SEE_VALUES[6] = { 153, 318, 319, 461, 979, 20000 }; // PBNRQK
constexpr int SEE_DEPTH_MAX = 8;

extern thread_local uint64_t nodes;

struct GoParams;

Move search(Board& board, int max_depth, int& best_score, const GoParams& params);

#endif // SEARCH_H