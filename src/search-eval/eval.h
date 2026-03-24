#ifndef EVAL_H
#define EVAL_H

#include "board/board.h"

constexpr int SCORE_MAX = 32000;

int eval(const Board& position);
int eval_perspective(const Board& position);
void initEval();

#endif // EVAL_H