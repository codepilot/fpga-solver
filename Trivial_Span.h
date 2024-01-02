#pragma once

template<typename T> concept TrivialStandardLayout = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

template<TrivialStandardLayout T>
class Trivial_Span {
public:
    static_assert(std::is_trivial_v<T> == true);
    using pointer = T*;
    pointer data;
    size_t size;
    inline constexpr static Trivial_Span make(pointer data, size_t size) noexcept {
        return { .data{data}, .size{size} };
    }
    inline constexpr Trivial_Span subspan(int64_t offset, size_t size) noexcept {
        return { .data{data + offset}, .size{size} };
    }
    inline constexpr bool empty() noexcept {
        return nullptr == data;
    }
    inline constexpr pointer begin() noexcept {
        return data;
    }
    inline constexpr pointer end() noexcept {
        return data + size;
    }
};

#if defined(_WIN32)
using Trivial_Span__m256i = Trivial_Span<__m256i>;
#endif
using Trivial_Span_u64 = Trivial_Span<uint64_t>;
using Trivial_Span_cu64 = Trivial_Span<const uint64_t>;
#if defined(_WIN32)
static_assert(std::is_trivial_v<Trivial_Span__m256i>&& std::is_standard_layout_v<Trivial_Span__m256i>);
#endif
static_assert(std::is_trivial_v<Trivial_Span_u64>&& std::is_standard_layout_v<Trivial_Span_u64>);
static_assert(std::is_trivial_v<Trivial_Span_cu64>&& std::is_standard_layout_v<Trivial_Span_cu64>);
#if defined(_WIN32)
static_assert(sizeof(Trivial_Span__m256i) == 16);
#endif
static_assert(sizeof(Trivial_Span_u64) == 16);
