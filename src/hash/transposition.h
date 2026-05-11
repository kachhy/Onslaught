#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H
#include "board/board.h"
#include <memory>

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
constexpr size_t DEFAULT_TABLE_MB  = 16;

class TTable {
private:
    std::unique_ptr<EntryTriple[]> table;
    size_t table_capacity; // number of buckets, always a power of 2
    size_t index_mask;     // table_capacity - 1
    size_t table_age;
    size_t table_size;     // populated entry count
public:
    explicit TTable(size_t megabytes = DEFAULT_TABLE_MB);
    void resize(size_t megabytes);
    void insert(const Board& board, Move best_move, int32_t score, TTBound bound, size_t depth);
    bool fetch(const Board& board, Entry& entry);
    void clear();
    inline size_t size() const { return table_size; }
    inline size_t capacity() const { return table_capacity; }
    inline void incAge() { table_age++; }
};
extern TTable tt;
#endif // TRANSPOSITION_H