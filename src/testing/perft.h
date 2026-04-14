#ifndef PERFT_H
#define PERFT_H

#include "board/board.h"
#include "movegen/movegen.h"
#include <cassert>

void runTimePerftTest(int depth, const unsigned long long expected[], const std::string& fen = "");
void perftTests();
void bench();

#endif // PERFT_H