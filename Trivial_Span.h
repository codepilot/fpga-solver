#pragma once

template<typename T> concept TrivialStandardLayout = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

template<TrivialStandardLayout T>
class Trivial_Span {
public:
    static_assert(std::is_trivial_v<T> == true);
    using pointer = T*;
    pointer data;
    size_t size;
    __forceinline constexpr static Trivial_Span make(pointer data, size_t size) noexcept {
        return { .data{data}, .size{size} };
    }
    __forceinline constexpr Trivial_Span subspan(int64_t offset, size_t size) noexcept {
        return { .data{data + offset}, .size{size} };
    }
    __forceinline constexpr bool empty() noexcept {
        return nullptr == data;
    }
    __forceinline constexpr pointer begin() noexcept {
        return data;
    }
    __forceinline constexpr pointer end() noexcept {
        return data + size;
    }
};

using Trivial_Span__m256i = Trivial_Span<__m256i>;
using Trivial_Span_u64 = Trivial_Span<uint64_t>;
using Trivial_Span_cu64 = Trivial_Span<const uint64_t>;
static_assert(std::is_trivial_v<Trivial_Span__m256i>&& std::is_standard_layout_v<Trivial_Span__m256i>);
static_assert(std::is_trivial_v<Trivial_Span_u64>&& std::is_standard_layout_v<Trivial_Span_u64>);
static_assert(std::is_trivial_v<Trivial_Span_cu64>&& std::is_standard_layout_v<Trivial_Span_cu64>);
static_assert(sizeof(Trivial_Span__m256i) == 16);
static_assert(sizeof(Trivial_Span_u64) == 16);
