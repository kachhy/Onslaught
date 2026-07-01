#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"
#include <chrono>
#include <cstddef>

constexpr uint8_t RFP_MARGIN = 84;
constexpr uint8_t ASPIRATION_MARGIN = 20;
constexpr uint16_t FUTILITY_MARGIN = 90;
constexpr uint16_t RAZOR_MARGIN = 492;
constexpr float LMR_VALUE = 0.828061;
constexpr float LMR_SCALAR = 2.172229;
constexpr int HIST_LMR_DIVISOR = 8192; // history score -> LMR reduction adjustment
constexpr uint8_t NMP_DEPTH_CUTOFF = 3;
constexpr uint8_t RAZORING_DEPTH_MAX = 2;
constexpr uint8_t IIR_DEPTH_CUTOFF = 3;
constexpr uint8_t FP_DEPTH_MAX = 4;
constexpr uint8_t LMP_DEPTH_MAX = 8;
constexpr int LMP_BASE = 3;
constexpr uint8_t LMR_MOVES_CUTOFF = 6;
constexpr uint8_t LMR_DEPTH_CUTOFF = 1;
constexpr uint8_t ASPIRATION_DEPTH_CUTOFF = 3;
constexpr float ASPIRATION_SCALAR = 1.290771;
constexpr int SEE_VALUES[6] = { 100, 300, 300, 500, 900, 20000 }; // PBNRQK
constexpr int SEE_DEPTH_MAX = 8;
constexpr int PROB_BETA_OFFSET = 180;

extern thread_local uint64_t nodes;

struct GoParams;

Move search(Board& board, int max_depth, int& best_score, const GoParams& params);

#endif // SEARCH_H