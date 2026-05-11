#include "uci.h"
#include "hash/transposition.h"
#include "search-eval/search.h"
#include "tuning/spsa.h"
#include <algorithm>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

Board board;
bool searching = false;
bool debug_mode = false;
std::unordered_map<std::string, struct OptionVar> options_map;

void setOptions(std::string key, struct OptionVar value) { options_map[key] = value; }

// All the different things we can change about the engine
static inline void options() {
    std::vector<std::string> keys;
    keys.reserve(options_map.size());
    for (const auto& [key, _] : options_map) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
        const auto& value = options_map[key];
        std::cout << "option name " << key << " type spin default " << value.norm << " min " << value.min << " max " << value.max << "\n";
    }
}

// actually changing the options for the engine
static inline void changeOptions(std::istringstream& ss) {
    std::string token;

    if (!(ss >> token) || token != "name") {
        return;
    }

    std::string option_name;
    while (ss >> token && token != "value") {
        if (!option_name.empty()) {
            option_name += " ";
        }
        option_name += token;
    }
    if (token != "value" || option_name.empty()) {
        return;
    }

    std::string value;
    if (!(ss >> value)) {
        return;
    }

    auto it = options_map.find(option_name);
    if (it == options_map.end()) {
        if (debug_mode) {
            std::cout << "info string Unknown option " << option_name << std::endl;
        }
        return;
    }

    if (!it->second.setter) {
        if (debug_mode) {
            std::cout << "info string Option " << option_name << " not set due to null setter" << std::endl;
        }
        return;
    }

    try {
        it->second.setter(std::stoi(value));
        if (debug_mode) {
            std::cout << "info string Option " << option_name << " set to " << value << std::endl;
        }
    } catch (const std::exception&) {
        if (debug_mode) {
            std::cout << "info string Invalid value '" << value << "' for option " << option_name << std::endl;
        }
    }
}

// called when a new game is started, resets the bot to its original state
static inline void newGame(Board& board) {
    // reset the board to the starting position
    board.clear();
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static inline void initOptions() {
    setOptions("Hash", { 1, 16384, 16, [](int mb) { tt.resize(mb); } });
    setOptions("Threads", { 1, 1, 1, nullptr });
    initSPSA();
}

static inline void position(Board& board, std::istringstream& ss) {
    std::string token;
    ss >> token;

    if (token == "startpos") {
        newGame(board);
        ss >> token; // consume "moves" if present
    } else if (token == "fen") {
        std::string fen;
        board.clear();
        while (ss >> token) {
            if (token == "moves") {
                break;
            }
            if (!fen.empty()) {
                fen += " ";
            }
            fen += token;
        }
        board.loadFEN(fen);
    }

    if (token == "moves") {
        while (ss >> token) {
            Move move = strToMove(token, board);
            board.makeMove(move);
        }
    }
}

static inline void go(Board& board, std::istringstream& ss) {
    GoParams params = GoParams();
    std::string token;

    while (ss >> token) {
        if (token == "wtime") {
            ss >> params.wtime;
        } else if (token == "btime") {
            ss >> params.btime;
        } else if (token == "winc") {
            ss >> params.winc;
        } else if (token == "binc") {
            ss >> params.binc;
        } else if (token == "movestogo") {
            ss >> params.movestogo;
        } else if (token == "depth") {
            ss >> params.depth;
        } else if (token == "nodes") {
            ss >> params.nodes;
        } else if (token == "movetime") {
            ss >> params.movetime;
        } else if (token == "infinite") {
            params.infinite = true;
        } else if (token == "ponder") {
            params.ponder = true;
        } else if (token == "searchmoves") {
            while (ss >> token) {
                params.searchmoves.push_back(token);
            }
        }
    }

    searching = true;
    int best_score;
    Move best_move = search(board, (params.infinite || params.depth == -1) ? 256 : params.depth, best_score, params);
    std::cout << "bestmove " << moveToStr(best_move) << std::endl;
}

void uci() {
    if (uciStartup() != 1) {
        return;
    }

    std::cout << "readyok" << std::endl;
    std::string line;

    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string buffer;
        ss >> buffer;

        if (buffer == "setoption") {
            changeOptions(ss);
        } else if (buffer == "go") {
            go(board, ss);
        } else if (buffer == "position") {
            position(board, ss);
        } else if (buffer == "ucinewgame") {
            newGame(board);
            tt.clear();
        } else if (buffer == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (buffer == "spsa") {
            printSPSAParams();
        } else if (buffer == "quit") {
            return;
        } else if (buffer == "debug") {
            ss >> buffer;
            if (buffer == "on") {
                debug_mode = true;
                std::cout << "info string Debug mode on" << std::endl;
            } else if (buffer == "off") {
                debug_mode = false;
                std::cout << "info string Debug mode off" << std::endl;
            }
        }
    }
}

int uciStartup() {
    std::string line;

    if (!std::getline(std::cin, line)) {
        return -1;
    }

    std::istringstream first(line);
    std::string buffer;
    first >> buffer;

    if (buffer != "uci") {
        std::cerr << "Expected 'uci' command, got '" << buffer << "'\n";
        return -1;
    }

    initOptions();
    std::cout << "id name Axiom\n";
    std::cout << "id author Connor Kostiew, Kai Chung, Will Bradley\n";
    options();
    std::cout << "uciok" << std::endl;

    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        ss >> buffer;

        if (buffer == "setoption") {
            changeOptions(ss);
        } else if (buffer == "isready") {
            return 1;
        } else if (buffer == "spsa") {
            printSPSAParams();
        } else if (buffer == "quit") {
            return 0;
        } else if (buffer == "debug") {
            ss >> buffer;
            if (buffer == "on") {
                debug_mode = true;
                std::cout << "info string Debug mode on" << std::endl;
            } else if (buffer == "off") {
                debug_mode = false;
                std::cout << "info string Debug mode off" << std::endl;
            }
        }
    }

    return -1;
}

static bool stdinHasData() {
#ifdef _WIN32
    DWORD n = 0;
    PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), nullptr, 0, nullptr, &n, nullptr);
    return n > 0;
#else
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    timeval tv{ 0, 0 };
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
#endif
}

void checkStdin(std::chrono::high_resolution_clock::time_point start, long long max_nodes, long long current_nodes, size_t hard_cap) {
    auto now = std::chrono::high_resolution_clock::now();
    if (hard_cap != 0 && (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= hard_cap) {
        searching = false;
        return;
    }
    if (max_nodes != -1 && current_nodes >= max_nodes) {
        searching = false;
        return;
    }
#ifndef SPSA_TUNE
    if (!stdinHasData()) {
        return;
    }
    std::string line;
    if (std::getline(std::cin, line)) {
        if (line == "stop") {
            searching = false;
        } else if (line == "quit") {
            exit(0);
        } else if (line == "debug on") {
            debug_mode = true;
            std::cout << "info string Debug mode on" << std::endl;
        } else if (line == "debug off") {
            debug_mode = false;
            std::cout << "info string Debug mode off" << std::endl;
        }
    }
#endif
}