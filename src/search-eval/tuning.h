#ifndef TUNING_H
#define TUNING_H

#include <fstream>
#include <vector>
#include <string>
#include "core/types.h"

struct Position {
    std::string fen;
    Side result;

    Position(const std::string& fen, const Side result) : fen(fen), result(result) {}
};

std::vector<Position> parseDataset(const std::string& filename);

#endif // TUNING_H