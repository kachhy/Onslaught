#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include "board.h"

struct Entry {
    uint64_t hash;
    Move best_move;
    int32_t score;
    TTBound bound;
    size_t depth, last_seen;
};

struct EntryTriple {
    Entry entries[3];
    size_t count; // Default 0 after clear()
};

constexpr size_t ENTRY_TRIPLE_SIZE = sizeof(EntryTriple);
constexpr size_t TABLE_SIZE        = (16 * 1024 * 1024) / ENTRY_TRIPLE_SIZE; // 16 MB

class TTable {
private:
    EntryTriple table[TABLE_SIZE];
    size_t table_age, table_size;
public:
    TTable();

    void insert(const Board& board, Move best_move, int32_t score, TTBound bound, size_t depth);
    bool fetch(const Board& board, Entry& entry);
    void clear();
    inline size_t size() const { return table_size; }
};

extern TTable tt;

#endif // TRANSPOSITION_H