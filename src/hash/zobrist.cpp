#include "zobrist.h"
#include "core/types.h"
#include "random.h"

uint64_t piece_keys[12][64];
uint64_t ep_keys[64];
uint64_t castle_keys[16];
uint64_t side_key;

void initZobrist() {
    RNGU64 rand_engine = RNGU64(DEFAULT_U64_SEED);

    for (uint8_t pc = static_cast<uint8_t>(WHITE_PAWN); pc <= static_cast<uint8_t>(BLACK_KING); pc++) {
        for (uint8_t sq = 0; sq < 64; sq++) {
            piece_keys[pc][sq] = rand_engine.next();
        }
    }

    for (uint8_t sq = 0; sq < 64; sq++) {
        ep_keys[sq] = rand_engine.next();
    }

    for (uint8_t sq = 0; sq < 16; sq++) {
        castle_keys[sq] = rand_engine.next();
    }

    side_key = rand_engine.next();
}