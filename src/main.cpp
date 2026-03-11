#include "board.h"
#include "movegen.h"
#include "zobrist.h"
#include "transposition.h"
#include "search.h"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iomanip>

unsigned long long perft_test(Board& board, int depth) {
    if (depth == 0) {
        return 1ULL;
    }
    unsigned long long nodes = 0;
    MoveList m = getLegalMoves(board);
    for (size_t i = 0; i < m.size(); i++) {
        board.makeMove(m[i]);
        nodes += perft_test(board, depth - 1);
        board.undoMove(m[i]);
    }
    return nodes;
}

void perftTests() {
    std::vector<unsigned long long> nodes_starter = {
        1ULL,
        20ULL,
        400ULL,
        8902ULL,
        197281ULL,
        4865609ULL,
        119060324ULL,
        3195901860ULL,
        84998978956ULL,
        2439530234167ULL,
        69352859712417ULL,
        2097651003696806ULL,
        62854969236701747ULL
    };
    std::vector<unsigned long long> nodes_position_5 = {
        1ULL,
        44ULL,
        1486ULL,
        62379ULL,
        2103487ULL,
        89941194ULL
    };
    std::cout << "\nStandard board start perft tests:\n";
    std::locale us_locale("en_US.UTF-8");
    std::cout.imbue(us_locale);
    for (int i = 0; i <= 6; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        Board testing_board_1;
        unsigned long long result = perft_test(testing_board_1, i);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "depth: " << std::setw(2) << i << " | expected: " << std::setw(13) <<  nodes_starter[i] << " | result: " << std::setw(13) << result << " | " << (result == nodes_starter[i] ? "PASS" : "FAIL") << " - Time: " << std::setw(8) << duration.count() << "ms\n";
    }
    std::cout << "\nPosition 5 perft tests (rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8):\n";
    std::cout.imbue(us_locale);
    for (int i = 0; i <= 5; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        Board testing_board_2("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
        unsigned long long result = perft_test(testing_board_2, i);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "depth: " << std::setw(2) << i << " | expected: " << std::setw(13) <<  nodes_position_5[i] << " | result: " << std::setw(13) << result << " | " << (result == nodes_position_5[i] ? "PASS" : "FAIL") << " - Time: " << std::setw(8) << duration.count() << "ms\n";
    }
}

void searchTests() {
    for (int i = 0; i <= 7; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        Board testing_board;
        int result = search(testing_board, i);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "depth: " << std::setw(2) << i << " | score: " << std::setw(13) << result << " | Time: " << std::setw(8) << duration.count() << "ms\n";
    }
}

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

    // Test make move and TT
    Entry dummy_entry;
    Board b2;
    tt.insert(b2, 0, 0, EXACTBOUND, 1);
    b2.printBoard();
    b2.makeMove(GenerateMove(E2, E4, WHITE_PAWN, 0));
    tt.insert(b2, 0, 0, EXACTBOUND, 1);
    b2.printBoard();
    b2.makeMove(GenerateMove(G8, F6, BLACK_KNIGHT, 0));
    b2.printBoard();
    b2.makeMove(GenerateMove(E4, E5, WHITE_PAWN, 0));
    tt.insert(b2, 0, 0, EXACTBOUND, 1);
    tt.insert(b2, 0, 0, EXACTBOUND, 1);
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
    assert(tt.fetch(b2, dummy_entry) == true && "TT hit failed.");
    b2.undoMove(GenerateMove(E4, E5, WHITE_PAWN, 0));
    b2.printBoard();
    assert(tt.fetch(b2, dummy_entry) == false && "TT hit succeeded and should not have.");
    b2.undoMove(GenerateMove(G8, F6, BLACK_KNIGHT, 0));
    b2.printBoard();
    assert(tt.fetch(b2, dummy_entry) == true && "TT hit failed.");
    b2.undoMove(GenerateMove(E2, E4, WHITE_PAWN, 0));
    b2.printBoard();
    assert(tt.fetch(b2, dummy_entry) == true && "TT hit failed.");

    std::cout << tt.size() << std::endl;
    tt.clear();
    std::cout << tt.size() << std::endl;
    
    

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
    perftTests();
    searchTests();

    return 0;
}