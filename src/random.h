#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>
#include <random>

constexpr inline uint64_t DEFAULT_U64_SEED = 1804289383ULL;

// TODO: check if a xoshiro256** generator is faster and better for our use case
class RNGU64 {
private:
    std::mt19937_64 engine; 
public:
    RNGU64(uint64_t seed) : engine(seed) {}
    uint64_t next();
};

#endif // RANDOM_H