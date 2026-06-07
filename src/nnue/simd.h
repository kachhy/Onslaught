#ifndef SIMD_H
#define SIMD_H

// Defines a standard API for SIMD instructions.
// Currently implemented for AVX2, with a scalar fallback.

#include <cstdint>

// VEPI - vector of extended packed integer

#if defined(__AVX2__)
#include <immintrin.h>
using vepi16 = __m256i;
using vepi32 = __m256i;
constexpr int VEC_I16 = 16;
inline vepi16 vecLoad(const int16_t* p) noexcept { return _mm256_load_si256(reinterpret_cast<const __m256i*>(p)); }
inline void vecStore(int16_t* p, vepi16 v) noexcept { _mm256_store_si256(reinterpret_cast<__m256i*>(p), v); }

inline vepi16 vecAdd(vepi16 a, vepi16 b) noexcept { return _mm256_add_epi16(a, b); }
inline vepi16 vecSub(vepi16 a, vepi16 b) noexcept { return _mm256_sub_epi16(a, b); }
inline vepi16 vecMin(vepi16 a, vepi16 b) noexcept { return _mm256_min_epi16(a, b); }
inline vepi16 vecMax(vepi16 a, vepi16 b) noexcept { return _mm256_max_epi16(a, b); }
inline vepi16 vecMullo(vepi16 a, vepi16 b) noexcept { return _mm256_mullo_epi16(a, b); }

inline vepi32 vecMAdd(vepi16 a, vepi16 b) noexcept { return _mm256_madd_epi16(a, b); }
inline vepi32 vecAdd32(vepi32 a, vepi32 b) noexcept { return _mm256_add_epi32(a, b); }

inline vepi16 vecSet1(int16_t x) noexcept { return _mm256_set1_epi16(x); }
inline vepi32 vecZero32() noexcept { return _mm256_setzero_si256(); }

// Horizontal sum of 8x int32 -> scalar
inline int32_t vecReduce(vepi32 v) noexcept {
    __m128i lo = _mm256_castsi256_si128(v);      // lanes 0..3
    __m128i hi = _mm256_extracti128_si256(v, 1); // lanes 4..7
    __m128i s = _mm_add_epi32(lo, hi);           // 4x int32
    s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(1, 0, 3, 2)));
    s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
    return _mm_cvtsi128_si32(s);
}
#else // No SIMD - scalar fallback
using vepi16 = int16_t;
using vepi32 = int32_t;
constexpr int VEC_I16 = 1;
inline vepi16 vecLoad(const int16_t* p) noexcept { return *p; }
inline void vecStore(int16_t* p, vepi16 v) noexcept { *p = v; }

inline vepi16 vecAdd(vepi16 a, vepi16 b) noexcept { return static_cast<vepi16>(a + b); }
inline vepi16 vecSub(vepi16 a, vepi16 b) noexcept { return static_cast<vepi16>(a - b); }
inline vepi16 vecMin(vepi16 a, vepi16 b) noexcept { return a < b ? a : b; }
inline vepi16 vecMax(vepi16 a, vepi16 b) noexcept { return a > b ? a : b; }
inline vepi16 vecMullo(vepi16 a, vepi16 b) noexcept { return static_cast<vepi16>(a * b); }

inline vepi32 vecMAdd(vepi16 a, vepi16 b) noexcept { return static_cast<vepi32>(a) * static_cast<vepi32>(b); }
inline vepi32 vecAdd32(vepi32 a, vepi32 b) noexcept { return a + b; }

inline vepi16 vecSet1(int16_t x) noexcept { return x; }
inline vepi32 vecZero32() noexcept { return 0; }

inline int32_t vecReduce(vepi32 v) noexcept { return v; }
#endif

#endif // SIMD_H
