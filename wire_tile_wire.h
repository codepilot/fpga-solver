#pragma once

#include "PIP_Index.h"

#include "each.h"

#include "MemoryMappedFile.h"
#include <algorithm>
#include <cmath>
#include "interchange_types.h"

class WireTileWire {
public:
	/*
	
		devStrs: 467843, 19 bits
		tiles: 208370, 18 bits
		nodes: 28226432, 25 bits
		wires: 83282368, 27 bits
		alt_site_pins: 7926429, 23 bits
		max_pip_count: 4083, 12 bits

	uint64_t leaf : 1;
	uint64_t wire_branch : 25;
	uint64_t wireStrIdx : 19;
	uint64_t tileStrIdx : 19;
	*/

	uint64_t _value;

	inline constexpr uint64_t get_uint64_t() const noexcept {
        return _value;
    }

	inline constexpr uint64_t get_key() const noexcept {
        return _value / 83282368ull;
    }

	inline constexpr uint64_t get_tileStrIdx() const noexcept {
		return _value / (83282368ull * 467843ull);
	}

	inline constexpr uint64_t get_wireStrIdx() const noexcept {
		return (_value / 83282368ull) % 467843ull;
	}

	inline constexpr uint32_t get_wire_idx() const noexcept {
		return static_cast<uint32_t>(_value % 83282368ull);
	}

	inline constexpr static WireTileWire make(uint64_t tileStrIdx, uint64_t wireStrIdx, uint64_t wire_idx) noexcept {
		return { ._value{((tileStrIdx * (467843ull * 83282368ull)) + (wireStrIdx * 83282368ull) + wire_idx)} };
	}

};

static_assert(sizeof(WireTileWire) == sizeof(uint64_t));
static_assert(std::is_trivial_v<WireTileWire>);
static_assert(std::is_standard_layout_v<WireTileWire>);
static_assert(alignof(WireTileWire) == sizeof(uint64_t));

class Search_Wire_Tile_Wire {
public:

	MemoryMappedFile mmf_wire_tile_wire;
	std::span<WireTileWire> wire_tile_wire;

	Search_Wire_Tile_Wire():
		mmf_wire_tile_wire{ "sorted_wire_tile_wire.bin" },
		wire_tile_wire{ mmf_wire_tile_wire.get_span<WireTileWire>() }
	{
		puts(std::format("wire_tile_wire: {}, {} bits", wire_tile_wire.size(), ceil(log2(wire_tile_wire.size()))).c_str());
	}

	static std::span<WireTileWire> tile_to_wire(std::span<WireTileWire> wire_tile_wire, String_Index tileStrIdx) {
		auto key{ WireTileWire::make(tileStrIdx._strIdx, 0, 0) };
		return std::ranges::equal_range(wire_tile_wire, key, [](WireTileWire a, WireTileWire b) { return a.get_tileStrIdx() < b.get_tileStrIdx(); });
	}

	std::span<WireTileWire> tile_to_wire(String_Index tileStrIdx) const {
		return tile_to_wire(wire_tile_wire, tileStrIdx);
	}

	static uint32_t wire_tile_to_wire(std::span<WireTileWire> wire_tile_wire, String_Index tileStrIdx, String_Index wireStrIdx) {
		auto key{ WireTileWire::make(tileStrIdx._strIdx, wireStrIdx._strIdx, 0) };
		auto found{ std::ranges::equal_range(wire_tile_wire, key, [](WireTileWire a, WireTileWire b) { return a.get_key() < b.get_key(); }) };
		if (found.size() != 1) {
			DebugBreak();
		}
		for (auto&& entry : found) {
			if (tileStrIdx._strIdx != entry.get_tileStrIdx()) {
				abort();
			}
			if (wireStrIdx._strIdx != entry.get_wireStrIdx()) {
				abort();
			}
			return entry.get_wire_idx();
		}
		return UINT32_MAX;
	}

	uint32_t wire_tile_to_wire(String_Index tileStrIdx, String_Index wireStrIdx) const {
		return wire_tile_to_wire(wire_tile_wire, tileStrIdx, wireStrIdx);
	}

	static void make_wire_tile_wire(::DeviceResources::Device::Reader devRoot) {
		auto wires{ devRoot.getWires() };
		puts("make_wire_tile_wire() start");

		MemoryMappedFile mmf{ "sorted_wire_tile_wire.bin", wires.size() * sizeof(WireTileWire)};
		auto wire_tile_wire{ mmf.get_span<WireTileWire>() };

		puts(std::format("wire_tile_wire: {}, {} bits", wire_tile_wire.size(), ceil(log2(wire_tile_wire.size()))).c_str());

		each(wires, [&](uint64_t wire_idx, wire_reader wire) {
			wire_tile_wire[wire_idx] = WireTileWire::make(wire.getTile(), wire.getWire(), wire_idx);
		});

		puts(std::format("wire_tile_wire: {}, {} bits", wire_tile_wire.size(), ceil(log2(wire_tile_wire.size()))).c_str());

		puts("make_wire_tile_wire() sort");
		std::ranges::sort(wire_tile_wire, [](WireTileWire a, WireTileWire b) { return a.get_uint64_t() < b.get_uint64_t(); });

		puts("make_wire_tile_wire() finish");
	}

	void test(::DeviceResources::Device::Reader devRoot) const {
		puts("wire_tile_wire.test start");

		auto wires{ devRoot.getWires() };
		auto wire_count{ wires.size() };

		each(wires, [&](uint64_t wire_idx, wire_reader wire) {
			if (!(wire_idx % 100000)) puts(std::format("test {}%", static_cast<double_t>(wire_idx * 100ull) / static_cast<double_t>(wire_count)).c_str());
			auto found_wire_idx{ wire_tile_to_wire({ wire.getTile() }, { wire.getWire() }) };
			if (wire_idx != found_wire_idx) {
				puts(std::format("wire_idx: {}, found_wire_idx: {}", wire_idx, found_wire_idx).c_str());
				auto refind_found_wire_idx{ wire_tile_to_wire({ wire.getTile() }, { wire.getWire() }) };
				// abort();
			}
		});

		puts("wire_tile_wire.test finish");

	}

};