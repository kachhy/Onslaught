#ifndef HISTORY_H
#define HISTORY_H

#include "board/board.h"
#include "core/movelist.h"
#include "core/types.h"

constexpr int MAX_HISTORY = 8192;

extern int score_history[2][64][64]; // [stm][from][to] (butterfly history)

void resetHistory();
void updateScoreHistory(int depth, Side stm, Move move, const MoveList& quiets_tried);
int getScoreHistory(Side stm, Move move);

#endif // HISTORY_H