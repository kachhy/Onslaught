#include "mcts.h"
#include "uci/uci.h"
#include "core/move.h"
#include "core/movelist.h"
#include "movegen/movegen.h"
#include "board/rules.h"
#include "search.h"
#include <cmath>

//selects the best child node to explore next
mctsNode* selectNode(mctsNode* node, Board& board) {
    while (true) {
        MoveList legalMoves;
        getLegalMoves(board, legalMoves);

        if (node->children.size() < legalMoves.size()) {
            return node;
        }

        if (node->children.empty()) {
            return node;
        }

        mctsNode* bestChild = nullptr;
        double bestUCT = -std::numeric_limits<double>::infinity();

        for (mctsNode* child : node->children) {
            double uct;

            if (child->visits == 0) {
                uct = std::numeric_limits<double>::infinity();
            } else {
                double exploitation = child->value / child->visits;

                if (board.getSTM() == BLACK) {
                    exploitation = -exploitation;
                }

                double exploration = child->explorationConstant *
                    std::sqrt(std::log10(node->visits) / child->visits);

                uct = exploitation + exploration;
            }

            if (uct > bestUCT || bestChild == nullptr) {
                bestUCT = uct;
                bestChild = child;
            }
        }

        board.makeMove(bestChild->move);
        node = bestChild;
    }
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
//want to switch this to the alpha beta search we have
double simulate(Board& board, const GoParams& params) {
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
        int best_score;
        Move nextMove = search(board, (params.infinite || params.depth == -1) ? 256 : params.depth, best_score, params);
        board.makeMove(nextMove);
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

mctsNode* mostVisitedChild(mctsNode* node) {
    mctsNode* bestChild = nullptr;
    int maxVisits = -1;
    for (mctsNode* child : node->children) {
        if (child->visits > maxVisits) {
            maxVisits = child->visits;
            bestChild = child;
        }
    }
    return bestChild;
}

//The basic search and its loop for MCTS
Move searchMCTS(Board& board, int iterations, const GoParams& params) {
    //time management
    auto start = std::chrono::high_resolution_clock::now();
    size_t hard_cap = 0, soft_cap = 0; // 0 means no limit

    const int clock = board.getSTM() == WHITE ? params.wtime : params.btime;
    const int inc = board.getSTM() == WHITE ? params.winc : params.binc;

    if (params.movetime != -1) {
        hard_cap = std::max(params.movetime, 1);
        soft_cap = params.movetime;
    } else if (clock != -1) {
        const int64_t left = std::max(clock - move_overhead, 1);
        int64_t soft, hard;

        if (params.movestogo > 0) {
            soft = hard = left / params.movestogo + inc;
        } else {
            soft = left / 30 + inc / 3;
            hard = left / 20 + inc / 2;
        }

        hard = std::max<int64_t>(1, std::min(hard, left * 4 / 5));
        hard_cap = hard;
        soft_cap = std::clamp<int64_t>(soft, 1, hard);
    }

    GoParams searchParams = params;
    searchParams.movetime = soft_cap/30;
    searchParams.wtime = searchParams.btime = -1;
    searchParams.winc = searchParams.binc = 0;
    searchParams.movestogo = 0;
    mctsNode* root = new mctsNode(NO_MOVE, nullptr);
    for (int i = 0; i < iterations; i++) {
        // Stop the search if it has been interrupted
        checkStdin(start, iterations, i, hard_cap);
        if(!searching){
            break;
        }

        std::cout << "Info Iteration " << i << " best value " << (root->children.empty() ? 0 : mostVisitedChild(root)->value) << std::endl;
        mctsNode* node = selectNode(root, board);

        MoveList legalMoves;
        getLegalMoves(board, legalMoves);
        if (!legalMoves.empty()&&!isDraw(board, i)) {
            node = expandNode(node, board, legalMoves);
        }

        //gotta deal with time management
        double result = simulate(board, params);

        backpropagate(node, result);
    }
    std::cout << "Info Best childs value " << mostVisitedChild(root)->value << std::endl;
    return mostVisitedChild(root)->move;
}