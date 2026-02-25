#include "board.h"

int main() {
    // Board tests
    Board b; // ideally default position
    std::cout << "Starting position:\n";
    b.printBoard();

    std::cout << "Empty board:\n";
    b.clear();
    b.printBoard(); // Should be empty

    std::cout << "FEN Position 1:\n";
    b.loadFEN("r1b1r1k1/1pp2ppp/2n5/p1bqp3/8/P2P1NP1/1P2PPBP/R1BQ1RK1 w - - 0 11"); // Should now be some roughly even position
    b.printBoard();
    
    return 0;
}