#ifndef NNUE_H
#define NNUE_H

#include "accumulator.h"
#include <filesystem>

bool loadNNUE(const std::filesystem::path& path);
int evaluate(const Accumulator& accum, Side stm);

#endif // NNUE_H
