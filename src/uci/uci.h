#ifndef UCI_H
#define UCI_H

#include "board/board.h"
#include "core/move.h"
#include <iostream>
#include <string>



int uciStartup();
void uci();

//structs
struct GoParams {
    int wtime = -1, btime = -1;
    int winc = 0, binc = 0;
    int movestogo = 0; // 0 means unknown (sudden death)
    int depth = -1;
    long long nodes = -1;
    int movetime = -1;
    bool infinite = false;
    bool ponder = false;
    std::vector<std::string> searchmoves;
};
#endif // UCI_H