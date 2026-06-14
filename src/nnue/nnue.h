#ifndef NNUE_H
#define NNUE_H

#include "accumulator.h"
#include <cstddef>
#include <filesystem>

extern std::string nnue_path;
const std::string default_net("nn-1697fd3fc841dc25-v2.nnue");

// Embedded network symbols (defined in nnue.cpp via INCBIN or fallback stubs)
extern const unsigned char gNNUEWeightsData[];
extern const unsigned int  gNNUEWeightsSize;

bool loadNNUE(const std::filesystem::path& path);
bool loadNNUEFromMemory(const unsigned char* data, size_t size);
int evaluate(const Accumulator& accum, Side stm, int piece_count);

#endif // NNUE_H
