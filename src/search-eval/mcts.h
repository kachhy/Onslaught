#ifndef MCTS_H
#define MCTS_H

#include "board/board.h"

struct mctsNode {
    Move move;
    int visits = 0;
    double value = 0.0;
    std::vector<mctsNode*> children;
    mctsNode* parent = nullptr;

    mctsNode(Move m, mctsNode* p) : move(m), parent(p) {}
};

Move searchMCTS(Board& board, const GoParams& params);

#endif // MCTS_H