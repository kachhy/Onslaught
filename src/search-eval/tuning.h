#ifndef TUNING_H
#define TUNING_H

#include "search-eval/eval.h"
#include "core/types.h"
#include <fstream>
#include <string>
#include <vector>


struct Position {
    Board board;
    Side result;
    Position(const std::string& fen, const Side result) : board(fen), result(result) {}
};

std::vector<Position> parseDataset(const std::string& filename, const uint32_t max);
std::vector<Trace> precomputeTraces(const std::vector<Position>& positions);

#endif // TUNING_H