#ifndef RULES_H
#define RULES_H

#include "board.h"

bool isMaterialDraw(const Board& board);
bool isRepetitionDraw(const Board& board, uint32_t ply);
bool isFiftyMoveRuleDraw(Board& board);
bool isDraw(const Board& board, uint32_t ply);

#endif // RULES_H