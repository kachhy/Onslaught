#include "tuning.h"
#include <iostream>

std::vector<Position> parseDataset(const std::string& filename, const uint32_t max) {
    std::ifstream in(filename);
    std::string line;
    std::vector<Position> data;
    while (std::getline(in, line)) {
        const size_t index = line.find("[");
        const std::string res = line.substr(index + 1, 3);
        Side winner;
        if (res == "0.5") {
            winner = BOTH;
        } else if (res == "1.0") {
            winner = WHITE;
        } else {
            winner = BLACK;
        }
        data.emplace_back(line.substr(0, index), winner);
        if (!(data.size() % 100000)) {
            std::cout << "\tLoaded " << data.size() << " positions." << std::endl;

            if (data.size() >= max) {
                break;
            }
        }
    }
    return data;
}

std::vector<Trace> precomputeTraces(const std::vector<Position>& positions) {
    std::vector<Trace> traces;
    for (const Position& pos : positions) {
        trace = {};
        eval(pos.board);
        trace.phase = pos.board.phase();
        trace.result = pos.result;
        traces.emplace_back(trace);
    }
    return traces;
}