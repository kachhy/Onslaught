#include "search-eval/search.h"
#include <cassert>
#include <iomanip>
#include <chrono>
#include <unordered_map>

constexpr unsigned long long nodes_position_1[] = {
    1ULL,
    20ULL,
    400ULL,
    8902ULL,
    197281ULL,
    4865609ULL,
    119060324ULL,
    3195901860ULL, // 3 billion
    84998978956ULL, // 85 billion
    2439530234167ULL, // 2 trillion
    69352859712417ULL,
    2097651003696806ULL,
    62854969236701747ULL,
};

constexpr unsigned long long nodes_position_2[] = {
    1ULL,
    48ULL,
    2039ULL,
    97862ULL,
    4085603ULL,
    193690690ULL,
    8031647685ULL, // 8 billion
    374190009323ULL // 374 billion
};

constexpr unsigned long long nodes_position_3[] = {
    1ULL,
    14ULL,
    191ULL,
    2812ULL,
    43238ULL,
    674624ULL,
    11030083ULL,
    178633661ULL,
    3009794393ULL, // 3 billion
    50086749815ULL, // 50 billion
    860322602309ULL, // 860 bullion
};

constexpr unsigned long long nodes_position_4[] = {
    1ULL,
    6ULL,
    264ULL,
    9467ULL,
    422333ULL,
    15833292ULL,
    706045033ULL, // 700 million
    27209691363ULL, // 27 billion
    1209257875296ULL, // 1.2 tril - not stockfish tested
};

constexpr unsigned long long nodes_position_5[] = {
    1ULL,
    44ULL,
    1486ULL,
    62379ULL,
    2103487ULL,
    89941194ULL,
    3048196529ULL, // 3 billion
    131724123591ULL, // 131 bil - not stockfish tested
};

constexpr unsigned long long nodes_position_6[] = {
    1ULL,
    46ULL,
    2079ULL,
    89890ULL,
    3894594ULL,
    164075551ULL,
    6923051137ULL, // 6 billion
    287188994746ULL, // 287 billion
    11923589843526ULL,
    490154852788714ULL,
};

struct PerftKey {
    uint64_t hash;
    uint8_t depth;

    bool operator==(const PerftKey& other) const {
        return hash == other.hash && depth == other.depth;
    }
};

struct PerftKeyHasher {
    size_t operator()(const PerftKey& key) const {
        return std::hash<uint64_t>{}(key.hash ^ (static_cast<uint64_t>(key.depth) << 56));
    }
};

using PerftCache = std::unordered_map<PerftKey, unsigned long long, PerftKeyHasher>;

unsigned long long perft_test(Board& board, int depth, PerftCache& cache) {
    if (depth == 0) {
        return 1ULL;
    }
    unsigned long long nodes = 0;
    MoveList m = getLegalMoves(board);

    if (depth == 1) {
        return static_cast<unsigned long long>(m.size());
    }
    PerftKey key{board.hash(), static_cast<uint8_t>(depth)};
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }
    for (size_t i = 0; i < m.size(); i++) {
        board.makeMove(m[i]);
        nodes += perft_test(board, depth - 1, cache);
        board.undoMove(m[i]);
    }
    cache.emplace(key, nodes);
    return nodes;
}

unsigned long long perft_test_slow(Board& board, int depth) {
    if (depth == 0) {
        return 1ULL;
    }
    unsigned long long nodes = 0;
    MoveList m = getLegalMoves(board);
    for (size_t i = 0; i < m.size(); i++) {
        board.makeMove(m[i]);
        nodes += perft_test_slow(board, depth - 1);
        board.undoMove(m[i]);
    }
    return nodes;
}

unsigned long long divide(Board& board, int depth, PerftCache& cache) {
    if (depth == 0) {
        return 1ULL;
    }
    unsigned long long nodes = 0;
    MoveList m = getLegalMoves(board);
    for (size_t i = 0; i < m.size(); i++) {
        board.makeMove(m[i]);
        unsigned long long n = perft_test(board, depth - 1, cache);
        board.undoMove(m[i]);
        std::cout << moveToStr(m[i]) << ": " << n << "\n";
        nodes += n;
    }

    std::cout << "Total: " << nodes << "\n";
    return nodes;
}

void runTimePerftTest(int depth, const unsigned long long expected[], std::string fen = "") {
    if (fen == "") {
        std::cout << "\nPerft test with standard board:\n";
    } else {
        std::cout << "\nPerft test with board (" << fen << "):\n";
    }
    std::locale us_locale("en_US.UTF-8");
    std::cout.imbue(us_locale);
    for (int i = 0; i <= depth; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        Board testing_board_1;
        if (fen != "") {
            testing_board_1.loadFEN(fen);
        }
        PerftCache cache;
        unsigned long long result = perft_test(testing_board_1, i, cache);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "depth: " << std::setw(2) << i << " | expected: " << std::setw(18) <<  expected[i] << " | result: " << std::setw(18) << result << " | logical NPS: " << std::setw(10) << std::setprecision(3) << std::fixed << (static_cast<double>(result) / duration.count()) / static_cast<double>(1000) << " MNpS | computed NPS: " << std::setw(10) << std::setprecision(3) << std::fixed << (static_cast<double>(cache.size()) / duration.count()) / static_cast<double>(1000) << " MNpS | " << (result == expected[i] ? "PASS" : "FAIL") << " - Time: " << std::setw(10) << duration.count() << "ms\n";
    }
}

void perftTests() {
    std::vector<int> depths = {5, 3, 6, 4, 3 ,3};
    int length = 2; // 0 = instant, 1 = 2s, 2 = 20s, 3=300s, 4=idk 1hr+
    for (size_t i = 0; i < depths.size(); i++) {
        depths[i] += length;
    }
    runTimePerftTest(depths[0], nodes_position_1);
    runTimePerftTest(depths[1], nodes_position_2, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    runTimePerftTest(depths[2], nodes_position_3, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    runTimePerftTest(depths[3], nodes_position_4, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1");
    runTimePerftTest(depths[4], nodes_position_5, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    runTimePerftTest(depths[5], nodes_position_6, "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
}

void divideTests() {
    Board testing_board_1("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    int depth = 1;
    std:: cout << "divide tests (depth: " << depth << "):\n";
    PerftCache cache;
    divide(testing_board_1, depth, cache);
}

void searchTests() {
    std::cout << "\nSearch framework tests\n";
    for (int i = 0; i <= 8; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        Board testing_board;
        int score;
        Move result = search(testing_board, i, score);
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "depth: " << std::setw(2) << i << " | move: " << std::setw(13) << moveToStr(result) << " | score: " << std::setw(13) << score << " | Time: " << std::setw(8) << duration.count() << "ms\n";
    }
}

void initAttacks() {
    populateBetweenSquares();
    populateLineSquares();

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

    // Populate eval data
    initEval();
    
    // Run tests
    // tests();
    // perftTests();
    // divideTests();
    searchTests();

    return 0;
}