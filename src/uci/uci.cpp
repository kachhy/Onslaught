#include "uci.h"
#include "hash/transposition.h"
#include "search-eval/search.h"
#include <sstream>

Board board;

// All the different things we can change about the engine
static inline void options() {
    // currently does nothing for now but when we add options they will go here
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
        } else if (arg == "winc") {
            params.winc = std::stoi(buffer.substr(0, buffer.find(" ")));
            buffer = buffer.substr(buffer.find(" ") + 1);
        } else if (arg == "binc") {
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

    int best_score;
    Move best_move = search(board, params.depth, best_score);
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
