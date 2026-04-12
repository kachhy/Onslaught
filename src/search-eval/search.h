#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"

constexpr uint8_t RFP_MARGIN = 70;
constexpr uint8_t ASPIRATION_MARGIN = 25;
constexpr float LMR_VALUE = 1;
constexpr float LMR_SCALAR = 2;
constexpr uint8_t MAX_QPLY = 20;

int search(Board& board, int depth, int alpha, int beta, int ply = 0);
Move search(Board& board, int max_depth, int& best_score);

#endif // SEARCH_H