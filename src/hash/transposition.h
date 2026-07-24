#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H
#include "board/board.h"
#include <atomic>
#include <memory>

struct Entry {
    Move best_move;
    int16_t score;
    uint8_t bound, depth;
};

struct PackedEntry {
    uint16_t key;
    uint16_t move16; // from(6) | to(6)<<6 | flags(4)<<12 -- no piece/color
    int16_t score;
    uint8_t depth;
    uint8_t bound_age; // bound:2 (low bits), age:3, piece(DefaultPiece):3 (high bits)
};

constexpr uint8_t TT_AGE_BITS = 3;
constexpr uint8_t TT_AGE_MASK = (1u << TT_AGE_BITS) - 1;

struct alignas(64) EntryTriple {
    static constexpr uint8_t CAPACITY = 7; // (64 - sizeof(count)) / sizeof(PackedEntry)
    PackedEntry entries[CAPACITY];
    uint8_t count; // Default 0 after clear()
};

constexpr size_t ENTRY_TRIPLE_SIZE = sizeof(EntryTriple);
constexpr size_t DEFAULT_TABLE_MB = 256;

class TTable {
private:
    std::unique_ptr<EntryTriple[]> table;
    size_t table_capacity; // number of buckets, any size
    std::atomic<uint8_t> table_age;
    std::atomic<size_t> table_size; // populated entry count

    // Lemire multiply-shift
    inline size_t bucketIndex(const uint64_t hash) const { return static_cast<size_t>((static_cast<unsigned __int128>(hash) * table_capacity) >> 64); }

    // Verification key: low 16 bits
    static inline uint16_t keyOf(const uint64_t hash) { return static_cast<uint16_t>(hash); }

    static inline uint16_t packMove(Move move) { return static_cast<uint16_t>(From(move) | (To(move) << 6) | (Flags(move) << 12)); }

    static inline Move unpackMove(uint16_t move16, uint8_t piece_type, Side stm) {
        if (!move16) {
            return NO_MOVE;
        }

        const Square from = static_cast<Square>(move16 & 0x3f);
        const Square to = static_cast<Square>((move16 >> 6) & 0x3f);
        const uint32_t flags = (move16 >> 12) & 0xf;
        return GenerateMove(from, to, makePiece(static_cast<DefaultPiece>(piece_type), stm), flags);
    }

    static inline uint8_t boundOf(uint8_t bound_age) { return static_cast<uint8_t>(bound_age & 0x3); }
    static inline uint8_t ageOf(uint8_t bound_age) { return static_cast<uint8_t>((bound_age >> 2) & TT_AGE_MASK); }
    static inline uint8_t pieceOf(uint8_t bound_age) { return static_cast<uint8_t>((bound_age >> (2 + TT_AGE_BITS)) & 0x7); }
    static inline uint8_t packBoundAge(uint8_t bound, uint8_t age, uint8_t piece_type) {
        return static_cast<uint8_t>(boundOf(bound) | (age << 2) | (piece_type << (2 + TT_AGE_BITS)));
    }

    // Folds the non-key fields of an entry down to 16 bits, XORed into the
    // stored key so a torn read/write self-detects (lockless hashing trick).
    static inline uint16_t dataChecksum(uint16_t move16, int16_t score, uint8_t depth, uint8_t bound_age) {
        return static_cast<uint16_t>(move16 ^ static_cast<uint16_t>(score) ^ static_cast<uint16_t>((depth << 8) | bound_age));
    }

    static inline uint16_t encodeKey(uint16_t key, uint16_t move16, int16_t score, uint8_t depth, uint8_t bound_age) {
        return static_cast<uint16_t>(key ^ dataChecksum(move16, score, depth, bound_age));
    }

    static inline uint16_t decodeKey(const PackedEntry& e) { return static_cast<uint16_t>(e.key ^ dataChecksum(e.move16, e.score, e.depth, e.bound_age)); }

public:
    explicit TTable(size_t megabytes = DEFAULT_TABLE_MB);
    void resize(size_t megabytes);
    void insert(const Board& board, Move best_move, int16_t score, uint8_t bound, uint8_t depth);
    bool fetch(const Board& board, Entry& entry);
    void clear();
    inline void prefetch(const uint64_t hash) { __builtin_prefetch(&table[bucketIndex(hash)], 0, 1); }
    inline size_t size() const { return table_size; }
    inline size_t capacity() const { return table_capacity; }
    inline void incAge() { table_age = (table_age + 1) & TT_AGE_MASK; }
};

extern TTable tt;

#endif // TRANSPOSITION_H
