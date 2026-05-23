#ifndef RANDOM_H
#define RANDOM_H

#include <random>

constexpr uint64_t DEFAULT_U64_SEED = 1804289383ULL;

class RNGU64 {
private:
    std::mt19937_64 engine;
public:
    RNGU64(uint64_t seed) : engine(seed) { }
    uint64_t next();
};

#endif // RANDOM_H