#pragma once

#include <cinttypes>
#include <bit>
#include <nmmintrin.h>

class PIP_Index {
public:
    uint32_t _v;

    inline static constexpr PIP_Index make(uint32_t id, bool is_forward) noexcept {
        return { ._v{(id & 0x7fffffffu) | (static_cast<uint32_t>(is_forward) << 31u)}};// {._id{ id }, ._is_forward{ is_forward } };
    }
    inline static constexpr PIP_Index from_uint32_t(uint32_t value) noexcept {
        return std::bit_cast<PIP_Index, uint32_t>(value);
    }
    inline constexpr uint32_t as_uint32_t() const noexcept {
        return std::bit_cast<uint32_t, PIP_Index>(*this);
    }
    inline bool constexpr is_pip_forward() const noexcept {
        return static_cast<bool>(_v >> 31u);
    }
    inline bool constexpr is_root() const noexcept {
        return as_uint32_t() == UINT32_MAX;
    }
    inline constexpr uint32_t get_id() const noexcept {
        if (is_root()) {
            puts("get_id is root");
            abort();
        }
        return _v & 0x7fffffffu;
    }
    inline constexpr bool operator ==(const PIP_Index &b) const noexcept { return as_uint32_t() == b.as_uint32_t(); }
    inline constexpr bool operator !=(const PIP_Index &b) const noexcept { return as_uint32_t() != b.as_uint32_t(); }

};

inline static constexpr PIP_Index PIP_Index_Root{ PIP_Index::from_uint32_t(UINT32_MAX) };

template <>
struct std::hash<PIP_Index> {
    size_t operator()(const PIP_Index _Keyval) const noexcept {
        return _mm_crc32_u32(0, _Keyval.as_uint32_t());
    }
};


static_assert(sizeof(PIP_Index) == sizeof(uint32_t));
static_assert(std::is_trivial_v<PIP_Index>);
static_assert(std::is_standard_layout_v<PIP_Index>);

class PIP_Info {
public:
    uint64_t _wire0 : 28;
    uint64_t _reserved0 : 4;
    uint64_t _wire1 : 28;
    uint64_t _reserved1 : 3;
    uint64_t _directional : 1;

    inline constexpr uint32_t get_wire0() const noexcept { return static_cast<uint32_t>(_wire0); }
    inline constexpr uint32_t get_wire1() const noexcept { return static_cast<uint32_t>(_wire1); }
    inline constexpr bool is_directional() const noexcept { return static_cast<bool>(_directional); }
};

static_assert(sizeof(PIP_Info) == sizeof(uint64_t));
static_assert(std::is_trivial_v<PIP_Info>);
static_assert(std::is_standard_layout_v<PIP_Info>);

class String_Index {
public:
    uint32_t _strIdx;
    inline constexpr static uint64_t make_key(String_Index a, String_Index b) noexcept {
        return std::bit_cast<uint64_t, std::array<String_Index, 2>>({ a, b });
    }
    inline constexpr bool operator ==(const String_Index &b) const noexcept { return _strIdx == b._strIdx; }
    inline constexpr bool operator !=(const String_Index &b) const noexcept { return _strIdx != b._strIdx; }
    std::string_view get_string_view(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader strList) {
        return strList[_strIdx].cStr();
    }
};

template <>
struct std::hash<String_Index> {
    size_t operator()(const String_Index _Keyval) const noexcept {
        return _mm_crc32_u32(0, _Keyval._strIdx);
    }
};

static_assert(sizeof(String_Index) == sizeof(uint32_t));
static_assert(std::is_trivial_v<String_Index>);
static_assert(std::is_standard_layout_v<String_Index>);

class Wire_Info {
public:
    alignas(sizeof(uint64_t))
    String_Index _wire_strIdx;
    String_Index _tile_strIdx;

    inline constexpr String_Index get_wire_strIdx() const noexcept { return _wire_strIdx; }
    inline constexpr String_Index get_tile_strIdx() const noexcept { return _tile_strIdx; }
    inline constexpr uint64_t get_key() const noexcept {
        return String_Index::make_key(get_wire_strIdx(), get_tile_strIdx());
    }
};

static_assert(sizeof(Wire_Info) == sizeof(uint64_t));
static_assert(std::is_trivial_v<Wire_Info>);
static_assert(std::is_standard_layout_v<Wire_Info>);
static_assert(alignof(Wire_Info) == sizeof(uint64_t));


class Site_Pin_Info {
public:
    alignas(sizeof(std::array<uint32_t, 4>))
    String_Index _site;
    String_Index _site_pin;
    uint32_t _wire;
    uint32_t _node;
    inline constexpr uint64_t get_key() const noexcept {
        return String_Index::make_key(_site, _site_pin);
    }
    inline constexpr static uint64_t make_key(String_Index site_strIdx, String_Index site_pin_strIdx) noexcept {
        return String_Index::make_key(site_strIdx, site_pin_strIdx);
    }
    inline constexpr uint32_t get_wire_idx() const noexcept {
        return _wire;
    }
    inline constexpr uint32_t get_node_idx() const noexcept {
        return _node;
    }
};

static_assert(sizeof(Site_Pin_Info) == sizeof(std::array<uint32_t, 4>));
static_assert(std::is_trivial_v<Site_Pin_Info>);
static_assert(std::is_standard_layout_v<Site_Pin_Info>);
static_assert(alignof(Site_Pin_Info) == sizeof(std::array<uint32_t, 4>));

#include <bitset>
#include <memory>

using _NodeBitset = std::bitset<28226432ull>;
using _PipBitset = std::bitset<125372792ull>;
using upNodeBitset = std::unique_ptr<_NodeBitset>;
using upPipBitset = std::unique_ptr<_PipBitset>;
using rNodeBitset = _NodeBitset&;
using rPipBitset = _PipBitset&;
using pNodeBitset = _NodeBitset*;
using pPipBitset = _PipBitset*;
using cpNodeBitset = const _NodeBitset*;
using cpPipBitset = const _PipBitset*;
using crNodeBitset = const _NodeBitset&;
using crPipBitset = const _PipBitset&;

#if 0
static_assert(!std::is_trivial_v<NodeBitset>);
static_assert(!std::is_trivially_constructible_v<NodeBitset>);
static_assert(!std::is_trivially_default_constructible_v<NodeBitset>);

static_assert(std::is_trivially_copy_constructible_v<NodeBitset>);
static_assert(std::is_trivially_move_constructible_v<NodeBitset>);
static_assert(std::is_trivially_assignable_v<NodeBitset, NodeBitset>);
static_assert(std::is_trivially_copy_assignable_v<NodeBitset>);
static_assert(std::is_trivially_move_assignable_v<NodeBitset>);
static_assert(std::is_trivially_destructible_v<NodeBitset>);
static_assert(std::is_trivially_copyable_v<NodeBitset>);

static_assert(std::is_standard_layout_v<NodeBitset>);
#endif