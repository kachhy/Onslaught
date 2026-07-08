#ifndef MOVELIST_H
#define MOVELIST_H

#include "move.h"

constexpr uint8_t MAX_MOVES = 255; // Max discovered moves in a legal position is 218.

class MoveList {
public:
    struct iterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Move;
        using difference_type = std::ptrdiff_t;
        using pointer = Move*;
        using reference = Move&;

        iterator(Move* ptr) : ptr(ptr) { }

        reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        iterator& operator++() {
            ++ptr;
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++ptr;
            return tmp;
        }
        iterator& operator--() {
            --ptr;
            return *this;
        }
        iterator operator--(int) {
            iterator tmp = *this;
            --ptr;
            return tmp;
        }

        iterator operator+(difference_type n) const { return iterator(ptr + n); }
        iterator operator-(difference_type n) const { return iterator(ptr - n); }
        difference_type operator-(const iterator& other) const { return ptr - other.ptr; }

        reference operator[](difference_type n) const { return ptr[n]; }

        bool operator==(const iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const iterator& other) const { return ptr != other.ptr; }
        bool operator<(const iterator& other) const { return ptr < other.ptr; }
        bool operator>(const iterator& other) const { return ptr > other.ptr; }
        bool operator<=(const iterator& other) const { return ptr <= other.ptr; }
        bool operator>=(const iterator& other) const { return ptr >= other.ptr; }
    private:
        Move* ptr;
    };

    struct const_iterator {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Move;
        using difference_type = std::ptrdiff_t;
        using pointer = const Move*;
        using reference = const Move&;

        const_iterator(const Move* ptr) : ptr(ptr) { }

        reference operator*() const { return *ptr; }
        pointer operator->() const { return ptr; }

        const_iterator& operator++() {
            ++ptr;
            return *this;
        }
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++ptr;
            return tmp;
        }
        const_iterator& operator--() {
            --ptr;
            return *this;
        }
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --ptr;
            return tmp;
        }

        const_iterator operator+(difference_type n) const { return const_iterator(ptr + n); }
        const_iterator operator-(difference_type n) const { return const_iterator(ptr - n); }
        difference_type operator-(const const_iterator& other) const { return ptr - other.ptr; }

        reference operator[](difference_type n) const { return ptr[n]; }

        bool operator==(const const_iterator& other) const { return ptr == other.ptr; }
        bool operator!=(const const_iterator& other) const { return ptr != other.ptr; }
        bool operator<(const const_iterator& other) const { return ptr < other.ptr; }
        bool operator>(const const_iterator& other) const { return ptr > other.ptr; }
        bool operator<=(const const_iterator& other) const { return ptr <= other.ptr; }
        bool operator>=(const const_iterator& other) const { return ptr >= other.ptr; }
    private:
        const Move* ptr;
    };

    MoveList() : count(0), sel_sort_index(0) { };

    uint8_t size() const { return count; }
    bool empty() const { return !count; }
    inline void clear() { count = 0; sel_sort_index = 0; }
    void sort_item(const uint8_t index);
    inline void emplace_back(const Move move) {
        list[count] = move;
        ++count;
    }

    // Iterators
    iterator begin() { return iterator(list); }
    iterator end() { return iterator(list + count); }
    const_iterator begin() const { return const_iterator(list); }
    const_iterator end() const { return const_iterator(list + count); }
    const_iterator cbegin() const { return const_iterator(list); }
    const_iterator cend() const { return const_iterator(list + count); }

    Move operator[](const uint8_t index) const { return list[index]; }
    Move& operator[](const uint8_t index) { return list[index]; }
private:
    Move list[MAX_MOVES]; // Pun intended
    uint8_t count, sel_sort_index;
};

class RootMoveList : public MoveList {
private:
    Move claimed_moves[MAX_MOVES];
    uint8_t claimed_count = 0;
public:
    inline void claim(const Move move) {
        claimed_moves[claimed_count++] = move;
    }

    inline bool is_claimed(const Move move) const {
        for (uint8_t i = 0; i < claimed_count; i++) {
            if (claimed_moves[i] == move) {
                return true;
            }
        }
        return false;
    }

    // Call once per depth, before searching for pv_idx 0.
    inline void reset_claims() {
        claimed_count = 0;
    }
};

#endif // MOVELIST_H
