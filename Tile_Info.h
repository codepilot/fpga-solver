#pragma once


struct Tile_Info {
	int64_t minCol; //0
	int64_t minRow; //0
	int64_t maxCol; //669
	int64_t maxRow; //310
	int64_t numCol; //670 10 bits
	int64_t numRow; //311 9 bits
	// total 208370 18 bits

	inline constexpr static Tile_Info make() noexcept {
		return {
			.minCol{ INT64_MAX },
			.minRow{ INT64_MAX },
			.maxCol{ INT64_MIN },
			.maxRow{ INT64_MIN },
			.numCol{},
			.numRow{},
		};
	}

	inline constexpr size_t size() const noexcept {
		return numCol * numRow;
	}

	inline constexpr void set_tile(std::span<uint32_t> sp_tile_drawing, int64_t col, int64_t row, uint32_t val) const noexcept {
		sp_tile_drawing[col + row * numCol] = val;
	}

	inline static Tile_Info get_tile_info(::capnp::List< ::DeviceResources::Device::Tile, ::capnp::Kind::STRUCT>::Reader tiles) noexcept {
		auto ret{ Tile_Info::make() };
		for (auto&& tile : tiles) {
			const auto tileName{ tile.getName() };
			const auto col{ tile.getCol() };
			const auto row{ tile.getRow() };
			ret.minCol = (col < ret.minCol) ? col : ret.minCol;
			ret.minRow = (row < ret.minRow) ? row : ret.minRow;
			ret.maxCol = (col > ret.maxCol) ? col : ret.maxCol;
			ret.maxRow = (row > ret.maxRow) ? row : ret.maxRow;
		}
		ret.numCol = ret.maxCol - ret.minCol + 1ull;
		ret.numRow = ret.maxRow - ret.minRow + 1ull;
		return ret;
	}

	inline constexpr std::vector<std::array<uint16_t, 2>> get_tile_locations() const {
		std::vector<std::array<uint16_t, 2>> ret;
		ret.reserve(numCol * numRow);
		for (uint16_t row{}; row < numRow; row++) {
			for (uint16_t col{}; col < numCol; col++) {
				ret.emplace_back(std::array<uint16_t, 2>{ col, row });
			}
		}
		return ret;
	}
};
