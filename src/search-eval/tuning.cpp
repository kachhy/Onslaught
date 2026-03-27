#include "tuning.h"
#include <iostream>

Tuner::Tuner(const size_t dataset_size) {
    dataset.reserve(dataset_size);
    traces.reserve(dataset_size);
}

void Tuner::loadDataset(const std::string& filename, const uint32_t max) {
    std::ifstream in(filename);
    std::string line;
    std::cout << "[Tune] Loading positions..." << std::endl;
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
        dataset.emplace_back(line.substr(0, index), winner);
        if (!(dataset.size() % 100000)) {
            std::cout << "\tLoaded " << dataset.size() << " positions." << std::endl;

            if (dataset.size() >= max) {
                break;
            }
        }
    }
    std::cout << "[Tune] Preprocessing evaluation traces..." << std::endl;
    for (const Position& pos : dataset) {
        trace = {};
        eval(pos.board);
        trace.phase = pos.board.phase();
        trace.result = pos.result;
        traces.emplace_back(trace);
    }
}
