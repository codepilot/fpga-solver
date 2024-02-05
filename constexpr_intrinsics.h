#pragma once

template<const uint32_t start, const uint32_t len>
static __forceinline constexpr uint64_t extract_bits(const uint64_t word) noexcept {
    if (std::is_constant_evaluated()) {
        constexpr uint64_t mask{ ((1ui64 << static_cast<uint64_t>(len)) - 1ui64) };
        return (word >> static_cast<uint64_t>(start)) & mask;

    }
    else {
        return _bextr_u64(word, start, len);
    }
}

template<const int32_t count>
static __forceinline constexpr uint64_t rotate_left(const uint64_t word) noexcept {
    if (std::is_constant_evaluated()) {
        return (uint64_t)(((word) << (count)) | ((word) >> (64i32 - (count))));

    }
    else {
        return _rotl64(word, count);
    }
}

#ifndef __clang__

static __forceinline constexpr __m256i constexpr_mm256_xor_epi64(__m256i a, __m256i b) noexcept {
    if (std::is_constant_evaluated()) {
        return {.m256i_u64{
            a.m256i_u64[0] ^ b.m256i_u64[0],
            a.m256i_u64[1] ^ b.m256i_u64[1],
            a.m256i_u64[2] ^ b.m256i_u64[2],
            a.m256i_u64[3] ^ b.m256i_u64[3],
        }};
    }
    else {
        return _mm256_xor_epi64(a, b);
    }
}

static __forceinline constexpr __m256i constexpr_mm256_broadcast_i64x2(__m128i a) noexcept {
    if (std::is_constant_evaluated()) {
        return { .m256i_u64{
            a.m128i_u64[0],
            a.m128i_u64[1],
            a.m128i_u64[0],
            a.m128i_u64[1],
        } };
    }
    else {
        return _mm256_broadcast_i64x2(a);
    }
}
#endif

#include <vector>
template<typename T> constexpr T or_reduce(std::vector<T> v) {
    T ret{ 0 };
    for (auto& n : v) { ret |= n; }
    return ret;
};