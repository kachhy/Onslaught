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

    MoveList() : count(0), sel_sort_index(0) { };

    uint8_t size() const { return count; }
    bool empty() const { return !count; }
    void sort_item(const uint8_t item);
    inline void emplace_back(const Move move) {
        list[count] = move;
        ++count;
    }

    // Iterators
    iterator begin() { return iterator(list); }
    iterator end() { return iterator(list + count); }

    Move operator[](const uint8_t index) const { return list[index]; }
    Move& operator[](const uint8_t index) { return list[index]; }
private:
    Move list[MAX_MOVES]; // Pun intended
    uint8_t count, sel_sort_index;
};

#endif // MOVELIST_H