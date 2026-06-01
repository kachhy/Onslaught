#ifndef UCI_H
#define UCI_H

#include "board/board.h"
#include "core/move.h"
#include <chrono>
#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// Version descriptor
constexpr std::string_view version = "1.0";

// Variadic overload helper for std::visit
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// UCI option types
struct SpinOption {
    int min = 0;
    int max = 0;
    int norm = 0;
    std::function<void(int)> setter;
};

struct CheckOption {
    bool norm = true;
    std::function<void(bool)> setter;
};

struct StringOption {
    std::string norm;
    std::function<void(std::string)> setter;
};

using UCIOption = std::variant<SpinOption, CheckOption, StringOption>;

// Parsed setoption command
struct CmdSetOption {
    std::string name;
    std::optional<std::string> value;
};

// GoParams
struct GoParams {
    int wtime = -1, btime = -1;
    int winc = 0, binc = 0;
    int movestogo = 0; // 0 means unknown (sudden death)
    int depth = -1;
    long long nodes = -1;
    int movetime = -1;
    bool infinite = false;
    bool ponder = false;
    bool silent = false;
    std::vector<std::string> searchmoves;
};

int uciStartup();
void uci();
void checkStdin(std::chrono::high_resolution_clock::time_point start, long long max_nodes, long long current_nodes, size_t hard_cap);
void setOption(std::string key, UCIOption value);

extern thread_local bool searching;
extern thread_local bool stdin_enabled;
extern bool debug_mode;
extern std::unordered_map<std::string, UCIOption> options_map;

#endif // UCI_H
