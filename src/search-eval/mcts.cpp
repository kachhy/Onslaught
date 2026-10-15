#include "mcts.h"
#include "uci/uci.h"

Move searchMCTS(Board& board, const GoParams& params) {
    mctsNode* root = new mctsNode(NO_MOVE, nullptr);
    return NO_MOVE; // so that the code compiles while I go update the uci code to support MCTS
}