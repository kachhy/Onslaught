#include "mcts.h"
#include "uci/uci.h"
#include "core/move.h"
#include "core/movelist.h"
#include "movegen/movegen.h"
#include "board/rules.h"
#include "search.h"
#include "eval.h"
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
double simulate(Board& board, const GoParams& params, int ply) {
    Board* simulationBoard = new Board(board);
    while (true) {
        auto start = std::chrono::high_resolution_clock::now();
        if (isDraw(*simulationBoard, 0)) {
            std::cout << "DRAW DETECTED" << std::endl;
            delete simulationBoard;
            return 0;
        }

        MoveList legalMoves;
        getLegalMoves(*simulationBoard, legalMoves);

        if (legalMoves.empty()) {
            if (!simulationBoard->inCheck()) {
                std::cout << "STALEMATE DETECTED" << std::endl;
                delete simulationBoard;
                return 0;
            }

            std::cout << "CHECKMATE DETECTED" << std::endl;
            double result = simulationBoard->getSTM() == WHITE ? -1.0 : 1.0;
            delete simulationBoard;
            return result;
        }

        bool play_random_move = (rand() % 100) < 20;
        if(play_random_move){
            Move randomMove = legalMoves[rand() % legalMoves.size()];
            simulationBoard->makeMove(randomMove);
        } else {
            int bestScore = -std::numeric_limits<int>::max();
            Move bestMove = NO_MOVE;
            for(Move move : legalMoves){
                simulationBoard->makeMove(move);
                int value = quiesce(*simulationBoard, -SCORE_MAX, SCORE_MAX, ply, 0);
                if(value > bestScore){
                    bestScore = value;
                    bestMove = move;
                }
                simulationBoard->undoMove(move);

                checkStdin(start, 1, 0, params.movetime, false);
                if(!searching){
                    break;
                }
            }
            simulationBoard->makeMove(bestMove);
        }
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

void freeMCTSNode(mctsNode* node) {
    for (mctsNode* child : node->children) {
        freeMCTSNode(child);
    }
    delete node;
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

    GoParams simulateParams = params;
    simulateParams.movetime = soft_cap/30;
    simulateParams.wtime = simulateParams.btime = soft_cap/60;
    simulateParams.winc = simulateParams.binc = 0;
    simulateParams.movestogo = -1;
    mctsNode* root = new mctsNode(NO_MOVE, nullptr);
    for (int i = 0; i < iterations; i++) {
        // Stop the search if it has been interrupted
        checkStdin(start, iterations, i, hard_cap, true);
        if(!mcts_searching){
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
        double result = simulate(board, simulateParams, i);

        std::cout << "Iteration: " << i
          << " Simulation result: " << result
          << std::endl;


        searching = true;

        backpropagate(node, result);
        mctsNode* current = node;

        while (current->parent != nullptr) {
            board.undoMove(current->move);
            current = current->parent;
        }
    }
    if (root->children.empty()) {
        return NO_MOVE;
    }
    
    std::cout << "Info Best childs value " << mostVisitedChild(root)->value << std::endl;

    Move bestMove = mostVisitedChild(root)->move;
    freeMCTSNode(root);

    return bestMove;
}