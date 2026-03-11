#ifndef SEARCH_H
#define SEARCH_H

#include "movegen.h"
#include "eval.h"

int search(Board& board, int depth, int alpha = -SCORE_MAX, int beta = SCORE_MAX, int ply = 0);

#endif // SEARCH_H