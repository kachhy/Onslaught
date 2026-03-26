#include "tuning.h"

std::vector<Position> parseDataset(const std::string& filename) {
    std::ifstream in(filename);
    std::string line;
    std::vector<Position> data;
    while (std::getline(in, line)) {
        const size_t index = line.find("[");
        const std::string res = line.substr(index, 3);
        Side winner;
        if (res == "0.5") {
            winner = BOTH;
        } else if (res == "1.0") {
            winner = WHITE;
        } else {
            winner = BLACK;
        }
        data.emplace_back(line.substr(0, index), winner);
    }
    return data;
}