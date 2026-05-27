#ifndef NNUE_H
#define NNUE_H

#include "accumulator.h"
#include <cstddef>
#include <filesystem>

extern std::string nnue_path;

// Embedded network symbols (defined in nnue.cpp via INCBIN or fallback stubs)
extern const unsigned char gNNUEWeightsData[];
extern const unsigned int  gNNUEWeightsSize;

bool loadNNUE(const std::filesystem::path& path);
bool loadNNUEFromMemory(const unsigned char* data, size_t size);
int evaluate(const Accumulator& accum, Side stm);

#endif // NNUE_H
