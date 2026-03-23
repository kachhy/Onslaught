#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdint>

extern uint64_t piece_keys[12][64];
extern uint64_t ep_keys[64];
extern uint64_t castle_keys[16];
extern uint64_t side_key;

void initZobrist();

#endif // ZOBRIST_H