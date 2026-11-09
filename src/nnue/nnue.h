#ifndef NNUE_H
#define NNUE_H

#include "accumulator.h"
#include <cstddef>
#include <filesystem>

extern std::string nnue_path;
const std::string default_net("nn-0c9bc67b8c73abe8-v3.nnue");

// Embedded network symbols (defined in nnue.cpp via INCBIN or fallback stubs)
extern const unsigned char gNNUEWeightsData[];
extern const unsigned int  gNNUEWeightsSize;

bool loadNNUE(const std::filesystem::path& path);
bool loadNNUEFromMemory(const unsigned char* data, size_t size);
int evaluate(const Board& board);

#endif // NNUE_H
