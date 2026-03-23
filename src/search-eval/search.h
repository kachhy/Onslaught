#ifndef SEARCH_H
#define SEARCH_H

#include "../hash/transposition.h"
#include "../movegen/movegen.h"
#include "eval.h"

int search(Board& board, int depth, int alpha, int beta, int ply = 0);
Move search(Board& board, int depth, int& best_score);

#endif // SEARCH_H