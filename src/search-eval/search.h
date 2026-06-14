#ifndef SEARCH_H
#define SEARCH_H

#include "board/board.h"
#include <chrono>
#include <cstddef>

extern int RFP_MARGIN;
extern int ASPIRATION_MARGIN;
extern int FUTILITY_MARGIN;
extern int RAZOR_MARGIN;
extern float LMR_VALUE;
extern float LMR_SCALAR;
extern int HIST_LMR_DIVISOR; // history score -> LMR reduction adjustment
extern int NMP_DEPTH_CUTOFF;
extern int RAZORING_DEPTH_MAX;
extern int IIR_DEPTH_CUTOFF;
extern int FP_DEPTH_MAX;
extern int LMP_DEPTH_MAX;
extern int LMP_BASE;
extern int LMR_MOVES_CUTOFF ;
extern int LMR_DEPTH_CUTOFF;
extern int ASPIRATION_DEPTH_CUTOFF;
extern float ASPIRATION_SCALAR;
extern int SEE_VALUES[6]; // PBNRQK
extern int SEE_DEPTH_MAX;


extern thread_local uint64_t nodes;

struct GoParams;

Move search(Board& board, int max_depth, int& best_score, const GoParams& params);

#endif // SEARCH_H