#include "uci.h"
#include "hash/transposition.h"
#include "nnue/nnue.h"
#include "search-eval/eval.h"
#include "search-eval/search.h"
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

Board board;
thread_local bool searching = false;
thread_local bool stdin_enabled = true;
bool debug_mode = false;
std::unordered_map<std::string, UCIOption> options_map;

void setOption(std::string key, UCIOption value) { options_map[key] = std::move(value); }

// All the different things we can change about the engine
static inline void options() {
    for (const auto& [key, opt] : options_map) {
        std::visit(
            overloaded{ [&](const SpinOption& s) {
            std::cout << "option name " << key << " type spin default " << s.norm << " min " << s.min << " max " << s.max << "\n";
        }, [&](const CheckOption& c) { std::cout << "option name " << key << " type check default " << (c.norm ? "true" : "false") << "\n"; },
                        [&](const StringOption& s) { std::cout << "option name " << key << " type string default " << s.norm << "\n"; } },
            opt
        );
    }
}

static inline CmdSetOption parseSetOption(std::istream& is) {
    std::string token;
    is >> token;
    is >> token;
    CmdSetOption cmd{ token, std::nullopt };
    if (is >> token && token == "value") {
        is >> token;
        cmd.value = token;
    }
    return cmd;
}

static inline void applySetOption(const CmdSetOption& cmd) {
    if (!cmd.value) {
        return;
    }

    auto it = options_map.find(cmd.name);
    if (it == options_map.end()) {
        if (debug_mode) {
            std::cout << "info Option " << cmd.name << " not found\n";
        }
        return;
    }

    std::visit(
        overloaded{ [&](const SpinOption& s) {
        if (!s.setter) {
            if (debug_mode) {
                std::cout << "info Option " << cmd.name << " not set due to null setter\n";
            }
            return;
        }
        s.setter(std::stoi(*cmd.value));
        if (debug_mode) {
            std::cout << "info Option " << cmd.name << " set to " << *cmd.value << "\n";
        }
    },
                    [&](const CheckOption& c) {
        if (!c.setter) {
            if (debug_mode) {
                std::cout << "info Option " << cmd.name << " not set due to null setter\n";
            }
            return;
        }
        c.setter(*cmd.value == "true");
        if (debug_mode) {
            std::cout << "info Option " << cmd.name << " set to " << *cmd.value << "\n";
        }
    },
                    [&](const StringOption& s) {
        if (!s.setter) {
            if (debug_mode) {
                std::cout << "info Option " << cmd.name << " not set due to null setter\n";
            }
            return;
        }
        s.setter(*cmd.value);
        if (debug_mode) {
            std::cout << "info Option " << cmd.name << " set to " << *cmd.value << "\n";
        }
    } },
        it->second
    );
}

static inline void changeOptions() { applySetOption(parseSetOption(std::cin)); }

// called when a new game is started, resets the bot to its original state
static inline void newGame(Board& board) {
    // reset the board to the starting position
    board.clear();
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static inline void initOptions() {
    setOption("Hash", SpinOption{ 1, 16384, 256, [](int mb) { tt.resize(mb); } });
    setOption("Threads", SpinOption{ 1, 1, 1, nullptr });
    setOption("NNUE", CheckOption{ true, [](bool val) { use_nnue = val; } });
    setOption("EvalFile", StringOption{ "nn-0a63fbab92d2bb57-64.nnue", [](std::string path) {
        if (path == default_net && loadNNUEFromMemory(gNNUEWeightsData, gNNUEWeightsSize)) {
            std::cout << "info string NNUE eval by " << default_net << std::endl;
        } else {
            nnue_path = path;
            if (!loadNNUE(nnue_path)) {
                std::cout << "info string Unable to load " << nnue_path << std::endl;
            } else {
                std::cout << "info string NNUE eval by " << nnue_path << std::endl;
            };
        }
    } });
}

static inline void position(Board& board) {
    std::string line;
    std::getline(std::cin, line);
    std::istringstream ss(line);
    std::string token;

    ss >> token;

    if (token == "startpos") {
        newGame(board);
        ss >> token; // consume "moves" if present
    } else if (token == "fen") {
        std::string fen;
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

static inline void go(Board& board) {
    std::string buffer;
    std::string arg;
    GoParams params = GoParams();

    std::getline(std::cin, buffer);

    buffer = buffer + " ";

    arg = buffer.substr(0, buffer.find(" "));
    buffer = buffer.substr(buffer.find(" ") + 1);

    while (buffer.size() > 0) {
        arg = buffer.substr(0, buffer.find(" "));
        buffer = buffer.substr(buffer.find(" ") + 1);

        if (arg == "wtime") {
            params.wtime = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "btime") {
            params.btime = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "winc") { // time added after each move for white
            params.winc = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "binc") { // time added after each move for black
            params.binc = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "movestogo") {
            params.movestogo = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "depth") {
            params.depth = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "nodes") {
            params.nodes = std::stoll(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "movetime") {
            params.movetime = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "infinite") {
            params.infinite = true;
        } else if (arg == "ponder") {
            params.ponder = true;
        } else if (arg == "searchmoves") {
            while (buffer.size() > 0) {
                arg = buffer.substr(0, buffer.find(" "));
                buffer = buffer.substr(buffer.find(" ") + 1);
                params.searchmoves.push_back(arg);
            }
        }
    }

    searching = true;
    int best_score;
    Move best_move = search(board, (params.infinite || params.depth == -1) ? 256 : params.depth, best_score, params);
    std::cout << "bestmove " << moveToStr(best_move) << std::endl;
}

int uciStartup() {
    std::cout << std::unitbuf;
    std::string buffer;

    while (true) {
        std::cin >> buffer;
        if (buffer == "uci") {
            break;
        }
        buffer.clear();
    }

    initOptions();

    std::cout << "id name Axiom " << version << "\n";
    std::cout << "id author Connor Kostiew, Kai Chung, Will Bradley\n";
    options();
    std::cout << "uciok\n";

    // original setup loop
    while (1) {
        std::cin >> buffer;
        if (buffer == "setoption") {
            changeOptions();
        } else if (buffer == "isready") {
            return 1; // ready to start the game
        } else if (buffer == "quit") {
            return 0; // quit the engine
        }

        if (buffer == "debug") {
            std::cin >> buffer;
            if (buffer == "on") {
                debug_mode = true;
                std::cout << "info Debug mode on\n";
            } else if (buffer == "off") {
                debug_mode = false;
                std::cout << "info Debug mode off\n";
            }
        }
    }
}

void uci() {
    if (uciStartup() != 1) {
        return; // quit command received
    }

    std::cout << "readyok\n";
    std::string buffer;

    while (1) {
        std::cin >> buffer;

        if (buffer == "setoption") {
            changeOptions();
        } else if (buffer == "go") {
            go(board);
        } else if (buffer == "position") {
            position(board);
        } else if (buffer == "ucinewgame") {
            newGame(board);
            tt.clear();
        } else if (buffer == "isready") {
            std::cout << "readyok\n";
        } else if (buffer == "quit") {
            return; // quit the engine
        }

        if (buffer == "debug") {
            std::cin >> buffer;
            if (buffer == "on") {
                debug_mode = true;
                std::cout << "info Debug mode on\n";
            } else if (buffer == "off") {
                debug_mode = false;
                std::cout << "info Debug mode off\n";
            }
        }
    }
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

    if (!stdin_enabled) {
        return;
    }

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
            std::cout << "info Debug mode on\n";
        } else if (line == "debug off") {
            debug_mode = false;
            std::cout << "info Debug mode off\n";
        }
    }
}
