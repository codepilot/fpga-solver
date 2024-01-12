#pragma once

#include <cinttypes>
#include <bit>
#include <nmmintrin.h>
#include "interchange_types.h"
#include <cmath>

#ifdef _WIN32
#define always_inline inline __forceinline
#else
#define always_inline inline __attribute__((always_inline))
#endif

#define cai constexpr always_inline

#define isc inline static constexpr

#ifdef _WIN32
#define ccai cai
#else
#define ccai cai __attribute__((const))
#endif

class PIP_Index {
public:
    uint32_t _v;

    static ccai PIP_Index make(uint32_t id, bool is_forward) noexcept {
        return { ._v{(id & 0x7fffffffu) | (static_cast<uint32_t>(is_forward) << 31u)}};// {._id{ id }, ._is_forward{ is_forward } };
    }
    static ccai PIP_Index from_uint32_t(uint32_t value) noexcept {
        return std::bit_cast<PIP_Index, uint32_t>(value);
    }
    ccai uint32_t as_uint32_t() const noexcept {
        return std::bit_cast<uint32_t, PIP_Index>(*this);
    }
    bool ccai is_pip_forward() const noexcept {
        return static_cast<bool>(_v >> 31u);
    }
    bool ccai is_root() const noexcept {
        return as_uint32_t() == UINT32_MAX;
    }
    ccai uint32_t get_id() const noexcept {
        if (is_root()) {
            puts("get_id is root");
            abort();
        }
        return _v & 0x7fffffffu;
    }
    ccai bool operator ==(const PIP_Index &b) const noexcept { return as_uint32_t() == b.as_uint32_t(); }
    ccai bool operator !=(const PIP_Index &b) const noexcept { return as_uint32_t() != b.as_uint32_t(); }

};

isc PIP_Index PIP_Index_Root{ PIP_Index::from_uint32_t(UINT32_MAX) };

template <>
struct std::hash<PIP_Index> {
    always_inline size_t operator()(const PIP_Index _Keyval) const noexcept {
        return _mm_crc32_u32(0, _Keyval.as_uint32_t());
    }
};

static_assert(sizeof(PIP_Index) == sizeof(uint32_t));
static_assert(std::is_trivial_v<PIP_Index>);
static_assert(std::is_standard_layout_v<PIP_Index>);

class Tile_Index {
public:
    inline constexpr static int32_t cols{ 670u };
    inline constexpr static int32_t rows{ 311u };
    inline constexpr static int32_t count{ cols * rows };

    int32_t _value;
#if 0
    int16_t _col;
    int16_t _row;
#endif
    inline constexpr static Tile_Index make(int32_t x_col, int32_t y_row) noexcept {
        return {
            ._value{ x_col + (y_row * cols) },
#if 0
            ._col{static_cast<int16_t>(x_col)},
            ._row{static_cast<int16_t>(y_row)},
#endif
        };
    }
    inline static Tile_Index make(tile_reader tile) noexcept {
        return make(tile.getCol(), tile.getRow());
    }
    inline constexpr int32_t get_col() const noexcept {
        return _value % cols;
    }
    inline constexpr int32_t get_row() const noexcept {
        return _value / cols;
    }
    inline uint32_t manhattan_distance(const Tile_Index& b) const noexcept {
        return abs(get_col() - b.get_col()) + abs(get_row() - b.get_row());
    }
    inline double_t distance(const Tile_Index& b) const noexcept {
        return sqrt(pow(static_cast<double_t>(get_col() - b.get_col()), 2.0) + pow(static_cast<double_t>(get_row() - b.get_row()), 2.0));
    }
    ccai bool operator ==(const Tile_Index& b) const noexcept { return _value == b._value; }
    ccai bool operator !=(const Tile_Index& b) const noexcept { return _value != b._value; }
    ccai bool operator >(const Tile_Index& b) const noexcept { return _value > b._value; }
    ccai bool operator <(const Tile_Index& b) const noexcept { return _value < b._value; }
};

//static_assert(sizeof(Tile_Index) == sizeof(uint64_t));
static_assert(sizeof(Tile_Index) == sizeof(uint32_t));
static_assert(std::is_trivial_v<Tile_Index>);
static_assert(std::is_standard_layout_v<Tile_Index>);

class PIP_Info {
public:
    uint64_t _wire0 : 28;
    uint64_t _reserved0 : 4;
    uint64_t _wire1 : 28;
    uint64_t _reserved1 : 3;
    uint64_t _directional : 1;

    ccai uint32_t get_wire0() const noexcept { return static_cast<uint32_t>(_wire0); }
    ccai uint32_t get_wire1() const noexcept { return static_cast<uint32_t>(_wire1); }
    ccai bool is_directional() const noexcept { return static_cast<bool>(_directional); }
};

static_assert(sizeof(PIP_Info) == sizeof(uint64_t));
static_assert(std::is_trivial_v<PIP_Info>);
static_assert(std::is_standard_layout_v<PIP_Info>);

class String_Index {
public:
    uint32_t _strIdx;
    ccai static uint64_t make_key(String_Index a, String_Index b) noexcept {
        return std::bit_cast<uint64_t, std::array<String_Index, 2>>({ a, b });
    }
    ccai bool operator ==(const String_Index &b) const noexcept { return _strIdx == b._strIdx; }
    ccai bool operator !=(const String_Index &b) const noexcept { return _strIdx != b._strIdx; }
    always_inline std::string_view get_string_view(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader strList) {
        return strList[_strIdx].cStr();
    }
};

template <>
struct std::hash<String_Index> {
    always_inline size_t operator()(const String_Index _Keyval) const noexcept {
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

    ccai String_Index get_wire_strIdx() const noexcept { return _wire_strIdx; }
    ccai String_Index get_tile_strIdx() const noexcept { return _tile_strIdx; }
    ccai uint64_t get_key() const noexcept {
        return String_Index::make_key(get_wire_strIdx(), get_tile_strIdx());
    }
    always_inline static Wire_Info from_wire(DeviceResources::Device::Wire::Reader wire) noexcept {
        return Wire_Info{
                    ._wire_strIdx{._strIdx{wire.getWire()}},
                    ._tile_strIdx{._strIdx{wire.getTile()}},
        };
    }
    ccai bool operator ==(const Wire_Info& b) const noexcept { return get_key() == b.get_key(); }
    ccai bool operator !=(const Wire_Info& b) const noexcept { return get_key() != b.get_key(); }

    // always_inline Wire_Info(DeviceResources::Device::Wire::Reader wire) noexcept: Wire_Info{from_wire(wire)} { }
};

template <>
struct std::hash<Wire_Info> {
    always_inline size_t operator()(const Wire_Info _Keyval) const noexcept {
        return _mm_crc32_u64(0, _Keyval.get_key());
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
    ccai uint64_t get_key() const noexcept {
        return String_Index::make_key(_site, _site_pin);
    }
    ccai static uint64_t make_key(String_Index site_strIdx, String_Index site_pin_strIdx) noexcept {
        return String_Index::make_key(site_strIdx, site_pin_strIdx);
    }
    ccai uint32_t get_wire_idx() const noexcept {
        return _wire;
    }
    ccai uint32_t get_node_idx() const noexcept {
        return _node;
    }
};

static_assert(sizeof(Site_Pin_Info) == sizeof(std::array<uint32_t, 4>));
static_assert(std::is_trivial_v<Site_Pin_Info>);
static_assert(std::is_standard_layout_v<Site_Pin_Info>);
static_assert(alignof(Site_Pin_Info) == sizeof(std::array<uint32_t, 4>));

class Site_Pin_Tag {
public:
    alignas(sizeof(std::array<uint32_t, 2>))
    String_Index _site;
    String_Index _site_pin;
    ccai uint64_t get_key() const noexcept {
        return String_Index::make_key(_site, _site_pin);
    }
    ccai static uint64_t make_key(String_Index site_strIdx, String_Index site_pin_strIdx) noexcept {
        return String_Index::make_key(site_strIdx, site_pin_strIdx);
    }
};

static_assert(sizeof(Site_Pin_Tag) == sizeof(std::array<uint32_t, 2>));
static_assert(std::is_trivial_v<Site_Pin_Tag>);
static_assert(std::is_standard_layout_v<Site_Pin_Tag>);
static_assert(alignof(Site_Pin_Tag) == sizeof(std::array<uint32_t, 2>));

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

using branch_reader_pip_map = std::unordered_map<PIP_Index, branch_reader>;

#include <set>









