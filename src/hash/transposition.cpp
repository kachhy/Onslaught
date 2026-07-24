#include "transposition.h"
#include "uci/uci.h"
#include <cstring>

TTable tt;

static size_t computeCapacity(size_t megabytes) {
    if (megabytes == 0) {
        megabytes = 1;
    }

    size_t entries = (megabytes * MEGABYTE) / ENTRY_TRIPLE_SIZE;
    if (entries < 1) {
        entries = 1;
    }

    return entries; // Note that lemire doesn't require power of 2 rounding.
}

TTable::TTable(size_t megabytes) : table_age(0), table_size(0) {
    table_capacity = computeCapacity(megabytes);
    table.reset(new EntryTriple[table_capacity]()); // value-init -> zeroed
}

void TTable::resize(size_t megabytes) {
    if (searching) {
        return;
    }

    const size_t new_capacity = computeCapacity(megabytes);
    table.reset(new EntryTriple[new_capacity]());
    table_capacity = new_capacity;
    table_size = 0;
    table_age = 0;
}

void TTable::insert(const Board& board, Move best_move, int16_t score, uint8_t bound, uint8_t depth) {
    const uint16_t key = keyOf(board.hash());
    const uint16_t move16 = packMove(best_move);
    const uint8_t piece_type = (best_move == NO_MOVE) ? 0 : static_cast<uint8_t>(makeDefaultPiece(MovePiece(best_move)));
    const uint8_t bound_age = packBoundAge(bound, table_age, piece_type);
    const uint16_t stored_key = encodeKey(key, move16, score, depth, bound_age);
    EntryTriple& bucket = table[bucketIndex(board.hash())];

    if (bucket.count < EntryTriple::CAPACITY) {
        for (uint8_t i = 0; i < bucket.count; i++) {
            if (decodeKey(bucket.entries[i]) == key) {
                if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                    bucket.entries[i] = { stored_key, move16, score, depth, bound_age };
                }
                return;
            }
        }

        bucket.entries[bucket.count] = { stored_key, move16, score, depth, bound_age };
        bucket.count++;
        table_size++;
        return;
    }

    if (decodeKey(bucket.entries[0]) == key) {
        if (depth >= bucket.entries[0].depth || bound == EXACTBOUND) {
            bucket.entries[0] = { stored_key, move16, score, depth, bound_age };
        }
        return;
    }

    uint8_t kickout_index = 0;
    int64_t best_kickout_score = static_cast<int64_t>(bucket.entries[0].depth) - ((table_age - ageOf(bucket.entries[0].bound_age)) & TT_AGE_MASK);
    for (uint8_t i = 1; i < bucket.count; i++) {
        if (decodeKey(bucket.entries[i]) == key) {
            if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                bucket.entries[i] = { stored_key, move16, score, depth, bound_age };
            }

            return;
        }

        const int64_t this_kickout_score = static_cast<int64_t>(bucket.entries[i].depth) - ((table_age - ageOf(bucket.entries[i].bound_age)) & TT_AGE_MASK);
        if (this_kickout_score < best_kickout_score) {
            kickout_index = i;
            best_kickout_score = this_kickout_score;
        }
    }

    bucket.entries[kickout_index] = { stored_key, move16, score, depth, bound_age };
}

bool TTable::fetch(const Board& board, Entry& entry) {
    const uint16_t key = keyOf(board.hash());
    EntryTriple& bucket = table[bucketIndex(board.hash())];

    if (!bucket.count) {
        return false;
    }

    for (uint8_t i = 0; i < bucket.count; i++) {
        PackedEntry& packed = bucket.entries[i];
        if (decodeKey(packed) == key) {
            entry.best_move = unpackMove(packed.move16, pieceOf(packed.bound_age), board.getSTM());
            entry.score = packed.score;
            entry.depth = packed.depth;
            entry.bound = boundOf(packed.bound_age);

            if (ageOf(packed.bound_age) != table_age) {
                const uint8_t bound_age = packBoundAge(entry.bound, table_age, pieceOf(packed.bound_age));
                packed.key = encodeKey(key, packed.move16, packed.score, packed.depth, bound_age);
                packed.bound_age = bound_age;
            }

            return true;
        }
    }

    return false;
}

void TTable::clear() {
    static_assert(std::is_trivially_copyable<EntryTriple>::value, "EntryTriple must be trivial for memset to be safe.");
    memset(table.get(), 0, sizeof(EntryTriple) * table_capacity);
    table_size = 0;
    table_age = 0;
}
