#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H
#include "board/board.h"
#include <memory>

struct Entry {
    uint16_t hash; // Highest 16 bits of the Zobrist hash, disjoint from the key index.
    Move best_move;
    int16_t score;
    uint8_t bound, depth;
    uint16_t last_seen;
};

struct alignas(64) EntryTriple {
    Entry entries[3];
    uint8_t count; // Default 0 after clear()
};

constexpr size_t ENTRY_TRIPLE_SIZE = sizeof(EntryTriple);
constexpr size_t DEFAULT_TABLE_MB = 256;

class TTable {
private:
    std::unique_ptr<EntryTriple[]> table;
    size_t table_capacity; // number of buckets, any size
    uint16_t table_age;
    size_t table_size; // populated entry count

    // Lemire multiply-shift
    inline size_t bucketIndex(const uint64_t hash) const {
        return static_cast<size_t>((static_cast<unsigned __int128>(hash) * table_capacity) >> 64);
    }

    // Verification key: low 16 bits
    static inline uint16_t keyOf(const uint64_t hash) {
        return static_cast<uint16_t>(hash);
    }
public:
    explicit TTable(size_t megabytes = DEFAULT_TABLE_MB);
    void resize(size_t megabytes);
    void insert(const Board& board, Move best_move, int16_t score, uint8_t bound, uint8_t depth);
    bool fetch(const Board& board, Entry& entry);
    void clear();
    inline void prefetch(const uint64_t hash) { __builtin_prefetch(&table[bucketIndex(hash)], 0, 1); }
    inline size_t size() const { return table_size; }
    inline size_t capacity() const { return table_capacity; }
    inline void incAge() { table_age++; }
};

extern TTable tt;

#endif // TRANSPOSITION_H