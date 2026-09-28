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
    EntryTriple& bucket = table[bucketIndex(board.hash())];
    if (bucket.count < 3) {
        for (uint8_t i = 0; i < bucket.count; i++) {
            if (bucket.entries[i].hash == key) {
                if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                    bucket.entries[i] = { key, best_move, score, bound, depth, table_age };
                }
                return;
            }
        }

        bucket.entries[bucket.count] = { key, best_move, score, bound, depth, table_age };
        bucket.count++;
        table_size++;
        return;
    }

    if (bucket.entries[0].hash == key) {
        if (depth >= bucket.entries[0].depth || bound == EXACTBOUND) {
            bucket.entries[0] = { key, best_move, score, bound, depth, table_age };
        }
        return;
    }

    uint8_t kickout_index = 0;
    int64_t best_kickout_score = bucket.entries[0].depth - (table_age - bucket.entries[0].last_seen);
    for (uint8_t i = 1; i < bucket.count; i++) {
        if (bucket.entries[i].hash == key) {
            if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                bucket.entries[i] = { key, best_move, score, bound, depth, table_age };
            }
            return;
        }

        const int64_t this_kickout_score = bucket.entries[i].depth - (table_age - bucket.entries[i].last_seen);
        if (this_kickout_score < best_kickout_score) {
            kickout_index = i;
            best_kickout_score = this_kickout_score;
        }
    }

    bucket.entries[kickout_index] = { key, best_move, score, bound, depth, table_age };
}

bool TTable::fetch(const Board& board, Entry& entry) {
    const uint16_t key = keyOf(board.hash());
    EntryTriple& bucket = table[bucketIndex(board.hash())];

    if (!bucket.count) {
        return false;
    }

    for (uint8_t i = 0; i < bucket.count; i++) {
        if (bucket.entries[i].hash == key) {
            entry = bucket.entries[i];
            if (bucket.entries[i].last_seen != table_age) {
                bucket.entries[i].last_seen = table_age;
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