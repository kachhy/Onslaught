#include "board.h"
#include "zobrist.h"

void initAttacks() {
    populateBetweenSquares();

    populateBishopMasks();
    populateRookMasks();

    populatePawnAttacks();
    populateKingAttacks();
    populateKnightAttacks();
    populateBishopAttacks();
    populateRookAttacks();
}

void tests() {
    // // Test attacks
    // printBitboard(getPawnAttacks(E4, WHITE));
    // printBitboard(pawn_attacks[BLACK][H3]);
    // printBitboard(getKingAttacks(H2));
    // printBitboard(getKnightAttacks(E4));
    // printBitboard(knight_attacks[A1]);

    // // Board tests
    // Board b; // ideally default position
    // std::cout << "Starting position:\n";
    // b.printBoard();

    // std::cout << "Empty board:\n";
    // b.clear();
    // b.printBoard(); // Should be empty

    // std::cout << "FEN Position 1:\n";
    // b.loadFEN("r1b1r1k1/1pp2ppp/2n5/p1bqp3/8/P2P1NP1/1P2PPBP/R1BQ1RK1 w - - 0 11"); // Should now be some roughly even position
    // b.printBoard();

    // // Get rook atttacks from this board
    // printBitboard(getRookAttacks(F1, b.getOcc(BOTH)));
    // printBitboard(getRookAttacks(A1, b.getOcc(BOTH)));

    // // Get bishop attacks from this board
    // printBitboard(getBishopAttacks(C1, b.getOcc(BOTH)));
    // printBitboard(getBishopAttacks(G2, b.getOcc(BOTH)));

    // // Get queen attacks from this board
    // printBitboard(getQueenAttacks(D1, b.getOcc(BOTH)));

    // // Check EP square
    // b.loadFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    // b.printBoard();

    // Test make move
    Board b2;
    b2.printBoard();
    b2.makeMove(GenerateMove(E2, E4, WHITE_PAWN, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(G8, F6, BLACK_KNIGHT, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(E4, E5, WHITE_PAWN, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(D7, D5, BLACK_PAWN, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(E5, D6, WHITE_PAWN, CAPTURE_FLAG | EP_FLAG));
    b2.printBoard();
    b2.makeMove(GenerateMove(E7, E6, BLACK_PAWN, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(G1, F3, WHITE_KNIGHT, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(F8, D6, BLACK_BISHOP, CAPTURE_FLAG));
    b2.printBoard();
    b2.makeMove(GenerateMove(F1, E2, WHITE_BISHOP, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(E8, G8, BLACK_KING, CASTLE_FLAG));
    b2.printBoard();
    b2.makeMove(GenerateMove(E1, F1, WHITE_KING, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(E1, F1, WHITE_KING, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(E8, G8, BLACK_KING, CASTLE_FLAG));
    b2.printBoard();
    b2.undoMove(GenerateMove(F1, E2, WHITE_BISHOP, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(F8, D6, BLACK_BISHOP, CAPTURE_FLAG));
    b2.printBoard();
    b2.undoMove(GenerateMove(G1, F3, WHITE_KNIGHT, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(E7, E6, BLACK_PAWN, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(E5, D6, WHITE_PAWN, CAPTURE_FLAG | EP_FLAG));
    b2.printBoard();
    b2.undoMove(GenerateMove(D7, D5, BLACK_PAWN, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(E4, E5, WHITE_PAWN, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(G8, F6, BLACK_KNIGHT, 0));
    b2.printBoard();
    b2.undoMove(GenerateMove(E2, E4, WHITE_PAWN, 0));
    b2.printBoard();

    // Test random numbers
    // RNGU64 rand_engine = RNGU64(DEFAULT_U64_SEED);
    // for (uint8_t sq = 0; sq < 64; sq++) {
    //     std::cout << rand_engine.next() << std::endl;
    // }
}

int main() {
    // Populate attacks
    initAttacks();

    // Populate zobrist keys
    initZobrist();

    // Run tests
    tests();

    return 0;
}