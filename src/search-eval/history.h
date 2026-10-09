#ifndef HISTORY_H
#define HISTORY_H

#include "board/board.h"
#include "core/movelist.h"
#include "core/types.h"
#include "core/fast2darray.h"

constexpr int MAX_HISTORY = 8192;

extern int16_t score_history[2][64][64]; // [stm][from][to] (butterfly history)
extern int16_t cont_hist[2][6][64][6][64]; // [stm][prevPiece][prevTo][piece][to] (continuation history)

struct SearchStack {
    int static_eval;
    Move killers[2];
    Move move; // For continuation history - the current move
    Move excluded; // Singular-extension excluded move or NO_MOVE
};

constexpr int CORR_HIST_SIZE = 16384; // Must be a power of two
constexpr int CORR_HIST_SCALE = 256;
constexpr int CORR_MAX_BONUS = 2200;
constexpr int CORR_HIST_MAX = 16384;
extern thread_local FlattenedArray<int, CORR_HIST_SIZE, 2> kp_corrhist; // [pawn hash, stm]

void resetCorrHist();
int correctEval(const Board& board, int raw_eval);
void updateCorrHist(const Board& board, int raw_eval, int best_score, int depth);

void resetHistory();
void updateScoreHistory(int depth, Side stm, Move move, const MoveList& quiets_tried);
int getScoreHistory(Side stm, Move move);
void updateContHistory(int depth, Side stm, Move move, SearchStack* ss, const MoveList& quiets_tried);
int getContHist(SearchStack* ss, Side stm, Move move);

#endif // HISTORY_H