#ifndef HISTORY_H
#define HISTORY_H

#include "board/board.h"
#include "core/movelist.h"
#include "core/types.h"

constexpr int MAX_HISTORY = 8192;

extern int score_history[2][64][64]; // [stm][from][to] (butterfly history)
extern int cont_hist[2][6][64][6][64]; // [stm][prevPiece][prevTo][piece][to] (continuation history)

struct SearchStack {
    int static_eval;
    Move killers[2];
    Move move; // For continuation history - the current move
};

void resetHistory();
void updateScoreHistory(int depth, Side stm, Move move, const MoveList& quiets_tried);
int getScoreHistory(Side stm, Move move);
void updateContHistory(int depth, Side stm, Move move, SearchStack* ss, const MoveList& quiets_tried);
int getContHist(SearchStack* ss, Side stm, Move move);

#endif // HISTORY_H