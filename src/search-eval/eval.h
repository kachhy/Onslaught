#ifndef EVAL_H
#define EVAL_H

#include "board/rules.h"

constexpr int SCORE_MAX = 32000;

int eval(const Board& position);
void initEval();

#endif // EVAL_H