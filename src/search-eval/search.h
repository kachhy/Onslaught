#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"

int search(Board& board, int depth, int alpha, int beta, int ply = 0);
Move search(Board& board, int max_depth, int& best_score);

#endif // SEARCH_H