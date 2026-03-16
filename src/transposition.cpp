#include "transposition.h"

TTable tt;

TTable::TTable() : table_age(0) {
    clear();
}

void TTable::insert(const Board& board, Move best_move, int32_t score, TTBound bound, size_t depth) {
    const uint64_t hash = board.hash() % TABLE_SIZE;
    EntryTriple& bucket = table[hash];
    if (bucket.count < 3) {
        // Ensure entry doesn't already exist
        for (uint8_t i = 0; i < bucket.count; i++) {
            if (bucket.entries[i].hash == board.hash()) {
                if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                    bucket.entries[i] = {board.hash(), best_move, score, bound, depth, table_age};
                }
                return;
            }
        }

        bucket.entries[bucket.count] = {board.hash(), best_move, score, bound, depth, table_age};
        bucket.count++;
        table_size++; // We added a new entry
        return;
    }

    // Eviction policy
    Entry& best_kickout        = bucket.entries[0];
    int64_t best_kickout_score = bucket.entries[0].depth - (table_age - bucket.entries[0].last_seen);

    for (uint8_t i = 1; i < bucket.count; i++) {
        const int64_t this_kickout_score = bucket.entries[i].depth - (table_age - bucket.entries[i].last_seen);
        if (this_kickout_score < best_kickout_score) {
            best_kickout       = bucket.entries[i];
            best_kickout_score = this_kickout_score;
        }
    }

    best_kickout = {board.hash(), best_move, score, bound, depth, table_age};
}

bool TTable::fetch(const Board& board, Entry& entry) {
    const uint64_t hash = board.hash() % TABLE_SIZE;
    EntryTriple& bucket = table[hash];
    table_age++;

    if (!bucket.count) {
        return false;
    }

    // Check each hot element
    bool found = false;
    for (uint8_t i = 0; i < bucket.count; i++) {
        if (bucket.entries[i].hash == board.hash()) {
            entry = bucket.entries[i];
            bucket.entries[i].last_seen = table_age;
            found = true;
        }
    }

    return found;
}

void TTable::clear() {
    static_assert(std::is_trivially_copyable<EntryTriple>::value, "EntryTriple must be trivial for memset to be safe.");
    memset(table, 0, sizeof(table));
    table_size = 0;
}