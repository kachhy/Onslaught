#include "board.h"

void initAttacks() {
    populateBishopMasks();
    populateRookMasks();

    populatePawnAttacks();
    populateKingAttacks();
    populateKnightAttacks();
    populateBishopAttacks();
    populateRookAttacks();
}

int main() {
    // Populate attacks
    initAttacks();

    printBitboard(getPawnAttacks(E4, WHITE));
    printBitboard(pawn_attacks[BLACK][H3]);
    printBitboard(getKingAttacks(H2));
    printBitboard(getKnightAttacks(E4));
    printBitboard(knight_attacks[A1]);

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

    // Get rook atttacks from this board
    printBitboard(getRookAttacks(F1, b.getOcc(BOTH)));
    printBitboard(getRookAttacks(A1, b.getOcc(BOTH)));

    // Get bishop attacks from this board
    printBitboard(getBishopAttacks(C1, b.getOcc(BOTH)));
    printBitboard(getBishopAttacks(G2, b.getOcc(BOTH)));

    // Get queen attacks from this board
    printBitboard(getQueenAttacks(D1, b.getOcc(BOTH)));
    
    return 0;
}