#include "uci.h"

//All the different things we can change about the engine
static inline void options() {
    //currently does nothing for now but when we add options they will go here

}

//actually changing the options for the engine
static inline void changeOptions() {
    std::string token;
    std::cin >> token; // "name"
    std::cin >> token; // option name 

    //where all the checks for the different oprions will go
} 

//called when a new game is started, resets the bot to its original state
static inline void newGame(Board* board) {
    //reset the board to the starting position
    board->clear();
    board->loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static inline void position(Board* board){
    std::string buffer;
    std::string fen;
    std::getline(std::cin, buffer);

    fen = buffer.substr(0, buffer.find(" "));
    buffer = buffer.substr(buffer.find(" ") + 1);

    if (fen == "startpos") {
        newGame(board);
    }
    else if (fen == "fen") {
        fen = buffer.substr(0, buffer.find(" "));
        buffer = buffer.substr(buffer.find(" ") + 1);
        board->loadFEN(fen);
    }

    fen = buffer.substr(0, buffer.find(" "));
    buffer = buffer.substr(buffer.find(" ") + 1);

    if (fen == "moves"){
        while(buffer.size() > 0){
            fen = buffer.substr(0, buffer.find(" "));
            buffer = buffer.substr(buffer.find(" ") + 1);

            //waiting for strToMove to be made
            // Move *move = strToMove(fen);
            // board->makeMove(*move);
        }
    }
}

//not yet implemented, 
static inline void go(){
    
}

int uciStartup() {
    std::string buffer;
    std::cin >> buffer;

    if (buffer != "uci") {
       std::cerr << "Expected 'uci' command, got '" << buffer << "'\n";
       return -1; 
    }

    std::cout << "id name AXIOM\n";
    std::cout << "id author TBT\n";  // Agree on the authors name
    options();
    std::cout << "uciok\n";

    //original setup loop
    while(1){
        std::cin >> buffer;

        if (buffer == "setoption") {
            changeOptions();
        }
        else if (buffer == "isready") {
            return 1; //ready to start the game
        }
        else if (buffer == "quit") {
            return 0;//quit the engine
        }
    }
}

void uci() {
    if(uciStartup() != 1) {
        return; // quit command received
    }
    Board* board = new Board();
    std::cout << "readyok\n";

    std::string buffer;

    while(1){
        std::cin >> buffer;

        if (buffer == "setoption") {
            changeOptions();
        }
        else if(buffer == "go"){
            go();
        }
        else if (buffer == "position") {
            position(board);
        }
        else if (buffer == "ucinewgame") {
            newGame(board);
        }
        else if (buffer == "isready") {
            std::cout << "readyok\n";
        }
        else if (buffer == "quit") {
            delete board;
            return;//quit the engine
        }
    }

}