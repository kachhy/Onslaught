#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"

constexpr uint8_t RFP_MARGIN = 75;
constexpr uint8_t ASPIRATION_MARGIN = 25;
constexpr uint8_t FUTILITY_MARGIN = 100;
constexpr uint8_t RAZOR_MARGIN = 250;
constexpr float LMR_VALUE = 1;
constexpr float LMR_SCALAR = 2;
constexpr int MAX_HISTORY = 16384;

int search(Board& board, int depth, int alpha, int beta, int ply = 0);
Move search(Board& board, int max_depth, int& best_score);

#endif // SEARCH_H