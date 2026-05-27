#ifndef NNUE_H
#define NNUE_H

#include "accumulator.h"
#include <filesystem>

extern std::string nnue_path;

bool loadNNUE(const std::filesystem::path& path);
int evaluate(const Accumulator& accum, Side stm);

#endif // NNUE_H
