#ifndef FAST_2D_ARRAY_H
#define FAST_2D_ARRAY_H

#include <cassert>
#include <cstring>

template <typename T, size_t M, size_t N> // Array size MxN
class FlattenedArray {
public:
    explicit FlattenedArray() = default;

    void clear() {
        memset(arr, 0, sizeof(arr));
    }

    T& operator[](const size_t i, const size_t j) noexcept {
        assert(i < M);
        assert(j < N);
        return arr[j * M + i];
    }

    [[nodiscard]]
    const T& operator[](const size_t i, const size_t j) const noexcept {
        assert(i < M);
        assert(j < N);
        return arr[j * M + i];
    }
private:
    static_assert("FlattenedArray initialized to zero size." && (M != 0 || N != 0));
    T arr[M * N];
};

#endif // FAST_2D_ARRAY_H