#include "transposition.h"

TTable tt;

TTable::TTable() : table_age(0) {
    clear();
}

void TTable::insert(const Board& board, Move best_move, int32_t score, TTBound bound, size_t depth) {
    const uint64_t hash = board.hash() % TABLE_SIZE;

    if (table[hash].count < 3) {
        // Ensure entry doesn't already exist
        for (uint8_t i = 0; i < table[hash].count; i++) {
            if (table[hash].entries[i].hash == board.hash()) {
                return;
            }
        }

        table[hash].entries[table[hash].count] = {board.hash(), best_move, score, bound, depth, table_age};
        table[hash].count++;
        table_size++; // We added a new entry
        return;
    }

    // Eviction policy
    Entry& best_kickout        = table[hash].entries[0];
    int64_t best_kickout_score = table[hash].entries[0].depth - (table_age - table[hash].entries[0].last_seen);

    for (uint8_t i = 1; i < table[hash].count; i++) {
        const int64_t this_kickout_score = table[hash].entries[i].depth - (table_age - table[hash].entries[i].last_seen);
        if (this_kickout_score < best_kickout_score) {
            best_kickout       = table[hash].entries[i];
            best_kickout_score = this_kickout_score;
        }
    }

    best_kickout = {board.hash(), best_move, score, bound, depth, table_age};
}

bool TTable::fetch(const Board& board, Entry& entry) {
    const uint64_t hash = board.hash() % TABLE_SIZE;
    table_age++;

    if (!table[hash].count) {
        return false;
    }

    // Check each hot element
    bool found = false;
    for (uint8_t i = 0; i < table[hash].count; i++) {
        table[hash].entries[i].last_seen = table_age;
        if (table[hash].entries[i].hash == board.hash()) {
            entry = table[hash].entries[i];
            found = true;
        }
    }

    return found;
}

void TTable::clear() {
    static_assert(std::is_trivially_copyable_v<EntryTriple>, "EntryTriple must be trivial for memset to be safe.");
    memset(table, 0, sizeof(table));
    table_size = 0;
}