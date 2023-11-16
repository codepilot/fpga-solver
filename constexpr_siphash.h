#pragma once

class constexpr_siphash {
public:
	uint64_t hash;
	constexpr static uint64_t dROUNDS{ 4ui64 };

    constexpr static __m256i init{ .m256i_u64{0x736f6d6570736575ui64, 0x646f72616e646f6dui64, 0x6c7967656e657261ui64, 0x7465646279746573ui64} };

    __forceinline static constexpr __m256i round(__m256i v) noexcept {
        v.m256i_u64[0] += v.m256i_u64[1];
        v.m256i_u64[1] = rotate_left<13>(v.m256i_u64[1]);
        v.m256i_u64[1] ^= v.m256i_u64[0];
        v.m256i_u64[0] = rotate_left<32>(v.m256i_u64[0]);
        v.m256i_u64[2] += v.m256i_u64[3];
        v.m256i_u64[3] = rotate_left<16>(v.m256i_u64[3]);
        v.m256i_u64[3] ^= v.m256i_u64[2];
        v.m256i_u64[0] += v.m256i_u64[3];
        v.m256i_u64[3] = rotate_left<21>(v.m256i_u64[3]);
        v.m256i_u64[3] ^= v.m256i_u64[0];
        v.m256i_u64[2] += v.m256i_u64[1];
        v.m256i_u64[1] = rotate_left<17>(v.m256i_u64[1]);
        v.m256i_u64[1] ^= v.m256i_u64[2];
        v.m256i_u64[2] = rotate_left<32>(v.m256i_u64[2]);
        return v;
    }

    __forceinline static constexpr constexpr_siphash make(__m128i key, uint64_t word) noexcept {
        auto v{ constexpr_mm256_xor_epi64(init, constexpr_mm256_broadcast_i64x2(key)) };
        {
            v.m256i_u64[3] ^= word;
            v = round(round(v));
            v.m256i_u64[0] ^= word;
        }
        v = round(round(v));
        v.m256i_u64[2] ^= 0xff;
        v = round(round(round(round(v))));
        return { .hash{v.m256i_u64[0] ^ v.m256i_u64[1] ^ v.m256i_u64[2] ^ v.m256i_u64[3]} };
    }
#if 0
    __forceinline static constexpr constexpr_siphash make(__m128i key, Trivial_Span_cu64 words) noexcept {
        auto v{ constexpr_mm256_xor_epi64(init, constexpr_mm256_broadcast_i64x2(key)) };
        for (auto&& word : words) {
            v.m256i_u64[3] ^= word;
            v = round(round(v));
            v.m256i_u64[0] ^= word;
        }
        v = round(round(v));
        v.m256i_u64[2] ^= 0xff;
        v = round(round(round(round(v))));
        return { .hash{v.m256i_u64[0] ^ v.m256i_u64[1] ^ v.m256i_u64[2] ^ v.m256i_u64[3]}};
    }
#endif
};
static_assert(std::is_trivial_v<constexpr_siphash>&& std::is_standard_layout_v<constexpr_siphash>);
inline static constexpr std::array<uint64_t, 1> numbers{ 1235 };
inline static constexpr uint64_t t1{constexpr_siphash ::make(
    __m128i{.m128i_u64{0,0}},
    1235
).hash};