#ifndef MCTS_H
#define MCTS_H

#include "board/board.h"

struct mctsNode {
    Move move;
    int visits = 0;
    double value = 0.0;
    double explorationConstant = 1.414; // sqrt(2) for UCT formula

    std::vector<mctsNode*> children;
    mctsNode* parent = nullptr;

    mctsNode(Move m, mctsNode* p) : move(m), parent(p) {}
};

struct GoParams;

Move searchMCTS(Board& board, int iterations, const GoParams& params);

#endif // MCTS_H