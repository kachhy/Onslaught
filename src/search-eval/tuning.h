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

struct TunerParam {
    double value;
    double grad;

    // Adam moment values
    double m;
    double v;
};

class Tuner {
public:
    Tuner() = delete;
    Tuner(const size_t dataset_size);
    
    void loadDataset(const std::string& filename, const uint32_t max);
private:
    std::vector<Position> dataset;
    std::vector<Trace> traces;
};

#endif // TUNING_H