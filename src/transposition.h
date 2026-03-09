#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <string.h>
#include "board.h"

constexpr inline size_t TABLE_SIZE = 16 * 1024;

struct Entry {
    uint64_t hash;
    Move best_move;
    int32_t score;
    TTBound bound;
    size_t depth, last_seen;
};

class TTable {
public:
    struct EntryTriple {
        Entry entries[3];
        size_t count;

        EntryTriple() : count(0) {}
    };
private:
    EntryTriple table[TABLE_SIZE];
    size_t table_age;
public:
    TTable();

    void insert(const Board& board, Move best_move, int32_t score, TTBound bound, size_t depth);
    bool fetch(const Board& board, Entry& entry);
    void clear();
};

extern TTable tt;

#endif // TRANSPOSITION_H