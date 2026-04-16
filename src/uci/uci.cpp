#include "uci.h"
#include "hash/transposition.h"
#include "search-eval/search.h"
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/select.h>
    #include <unistd.h>
#endif

Board board;
bool searching = false;

// All the different things we can change about the engine
static inline void options() {
    // currently does nothing for now but when we add options they will go here
    std::cout << "option name Hash type spin default 16 min 16 max 16" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
}

// actually changing the options for the engine
static inline void changeOptions() {
    std::string token;
    std::cin >> token; // "name"
    std::cin >> token; // option name

    // where all the checks for the different oprions will go
}

// called when a new game is started, resets the bot to its original state
static inline void newGame(Board& board) {
    // reset the board to the starting position
    board.clear();
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
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
            if (token == "moves") break;
            if (!fen.empty()) fen += " ";
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

// not yet implemented,
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
        } else if (arg == "winc") {//time added after each move for white
            params.winc = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "binc") {//time added after each move for black
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
    std::string buffer;
    std::cin >> buffer;

    if (buffer != "uci") {
        std::cerr << "Expected 'uci' command, got '" << buffer << "'\n";
        return -1;
    }

    std::cout << "id name Axiom\n";
    std::cout << "id author Connor Kostiew, Kai Chung, Will Bradley\n"; // Agree on the authors name
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
    timeval tv{0, 0};
    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
#endif
}

void checkStdin( std::chrono::high_resolution_clock::time_point start, long long max_nodes, long long current_nodes, size_t hard_cap) {
    auto now = std::chrono::high_resolution_clock::now();
    if (hard_cap != 0 && (size_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= hard_cap) {
        searching = false;
        return;
    }
    if (max_nodes != -1 && current_nodes >= max_nodes) {
        searching = false;
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
        }
    }
}
