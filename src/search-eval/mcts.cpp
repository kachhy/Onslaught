#include "mcts.h"
#include "uci/uci.h"
#include "core/move.h"
#include "core/movelist.h"
#include "core/movegen.h"

//The basic search and its loop for MCTS
Move searchMCTS(Board& board, int iterations, const GoParams& params) {
    mctsNode* root = new mctsNode(NO_MOVE, nullptr);
    for (int i = 0; i < iterations; i++) {
        Board tempBoard = board; 

        mctsNode* node = selectNode(root, tempBoard);

        MoveList legalMoves;
        getLegalMoves(tempBoard, legalMoves);
        if (!legalMoves.empty()&&!isDraw(tempBoard, i)) {
            node = expandNode(node, tempBoard, legalMoves);
        }

        double result = simulate(tempBoard);

        backpropagate(node, result);
    }
    return mostVisitedChild(root)->move;
}

//selects the best child node to explore next
mctsNode* selectNode(mctsNode* node, Board& board) {
    while (!node->children.empty()) {
        node = node->children[0]; // Placeholder for selection logic
        board.makeMove(node->move);
    }
    return node;
}

//expands the node by adding a child for the next unexplored move and returns the new child node
mctsNode* expandNode(mctsNode* node, Board& board, MoveList legalMoves) {
    bool unexplored;
    for (const Move& move : legalMoves) {
        unexplored = true;

        for (mctsNode* child : node->children) {
            if (child->move == move) {
                unexplored = false;
                break; // Node already expanded for this move
            }
        }

        if (!unexplored) {
            continue;
        }

        board.makeMove(move);
        mctsNode* child = new mctsNode(move, node);
        node->children.push_back(child);
        return child;
    }
    return node; // All moves have been explored
}

//simulate a random playout from the current board state
//returns a value based on the outcome of the game (1 for win, 0 for loss, 0.5 for draw)
//current problem there is no determination of who won the game, also random moves suck.
double simulate(Board& board) {
    while (true) {
        if (isDraw(board, 0)) {
            return 0; // Draw
        }
        MoveList legalMoves;
        getLegalMoves(board, legalMoves);
        if (legalMoves.empty()) {
            if (!board.inCheck()) {
                return 0; // Stalemate
            }
            return board.getSTM() == 0 ? -1.0 : 1.0; // White wins or Black wins
        }
        Move randomMove = legalMoves[rand() % legalMoves.size()];
        board.makeMove(randomMove);
    }
}

//backpropagate the result of the simulation back up the tree
void backpropagate(mctsNode* node, double result) {
    while (node != nullptr) {
        node->visits++;
        node->value += result; // Assuming result is 1 for win, -1 for loss, 0 for draw
        node = node->parent;
    }
}