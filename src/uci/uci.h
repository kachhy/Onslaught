#ifndef UCI_H
#define UCI_H

#include "board/board.h"
#include "core/move.h"
#include <iostream>
#include <string>
#include <chrono>
#include <cstddef>
#include <unordered_map>
#include <functional>


int uciStartup();
void uci();
void checkStdin(std::chrono::high_resolution_clock::time_point start, long long max_nodes, long long current_nodes, size_t hard_cap);
void setOptions(std::string key, struct OptionVar value);

extern bool searching;
extern bool debug_mode;
extern std::unordered_map<std::string, struct OptionVar> options_map;

// structs
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

struct OptionVar {
    int min = 0;
    int max = 0;
    int norm = 0;
    std::function<void(int)> setter;
};

#endif // UCI_H
