#include "transposition.h"
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

    // Round down to nearest power of 2 so we can mask the hash for indexing.
    size_t pow2 = 1;
    while ((pow2 << 1) <= entries) {
        pow2 <<= 1;
    }
    return pow2;
}

TTable::TTable(size_t megabytes) : table_age(0), table_size(0) {
    table_capacity = computeCapacity(megabytes);
    index_mask = table_capacity - 1;
    table.reset(new EntryTriple[table_capacity]()); // value-init -> zeroed
}

void TTable::resize(size_t megabytes) {
    const size_t new_capacity = computeCapacity(megabytes);
    table.reset(new EntryTriple[new_capacity]());
    table_capacity = new_capacity;
    index_mask = new_capacity - 1;
    table_size = 0;
    table_age = 0;
}

void TTable::insert(const Board& board, Move best_move, int score, uint8_t bound, uint8_t depth) {
    const uint64_t hash = board.hash() & index_mask;
    const uint64_t pawn_hash = board.pawnHash() & index_mask;
    EntryTriple& bucket = table[hash];
    if (bucket.count < 3) {
        for (uint8_t i = 0; i < bucket.count; i++) {
            if (bucket.entries[i].hash == board.hash()) {
                if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                    bucket.entries[i] = { board.hash(), best_move, score, bound, depth, table_age };
                }
                return;
            }
        }
        bucket.entries[bucket.count] = { board.hash(), best_move, score, bound, depth, table_age };
        bloom_filter |= (1 << (hash & 31)) | (1 << (pawn_hash & 31));
        bucket.count++;
        table_size++;
        return;
    }
    if (bucket.entries[0].hash == board.hash()) {
        if (depth >= bucket.entries[0].depth || bound == EXACTBOUND) {
            bucket.entries[0] = { board.hash(), best_move, score, bound, depth, table_age };
            bloom_filter |= (1 << (hash & 31)) | (1 << (pawn_hash & 31));
        }
        return;
    }
    uint8_t kickout_index = 0;
    int64_t best_kickout_score = bucket.entries[0].depth - (table_age - bucket.entries[0].last_seen);
    for (uint8_t i = 1; i < bucket.count; i++) {
        if (bucket.entries[i].hash == board.hash()) {
            if (depth >= bucket.entries[i].depth || bound == EXACTBOUND) {
                bucket.entries[i] = { board.hash(), best_move, score, bound, depth, table_age };
                bloom_filter |= (1 << (hash & 31)) | (1 << (pawn_hash & 31));
            }
            return;
        }
        const int64_t this_kickout_score = bucket.entries[i].depth - (table_age - bucket.entries[i].last_seen);
        if (this_kickout_score < best_kickout_score) {
            kickout_index = i;
            best_kickout_score = this_kickout_score;
        }
    }
    bucket.entries[kickout_index] = { board.hash(), best_move, score, bound, depth, table_age };
    bloom_filter |= (1 << (hash & 31)) | (1 << (pawn_hash & 31));
}

bool TTable::fetch(const Board& board, Entry& entry) {
    const uint64_t hash = board.hash() & index_mask;
    const uint64_t pawn_hash = board.pawnHash() & index_mask;
    if (!(bloom_filter & (1 << (hash & 31)))
        || !(bloom_filter & (1 << (pawn_hash & 31)))) {
        return false;
    }
    EntryTriple& bucket = table[hash];
    if (!bucket.count) {
        return false;
    }
    bool found = false;
    for (uint8_t i = 0; i < bucket.count; i++) {
        if (bucket.entries[i].hash == board.hash()) {
            std::swap(bucket.entries[0], bucket.entries[i]);
            if (!found || bucket.entries[i].depth > entry.depth) {
                entry = bucket.entries[0];
            }
            bucket.entries[0].last_seen = table_age;
            found = true;
            break;
        }
    }
    return found;
}

void TTable::clear() {
    static_assert(std::is_trivially_copyable<EntryTriple>::value, "EntryTriple must be trivial for memset to be safe.");
    memset(table.get(), 0, sizeof(EntryTriple) * table_capacity);
    bloom_filter = 0;
    table_size = 0;
    table_age = 0;
}