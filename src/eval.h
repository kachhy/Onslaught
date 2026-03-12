#ifndef EVAL_H
#define EVAL_H

#include "board.h"

constexpr int SCORE_MAX = 32000;

extern int material_values[6];

int eval(const Board& position);

#endif // EVAL_H