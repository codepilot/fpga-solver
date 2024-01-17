#pragma once

void each(auto list, auto lambda) {
	uint64_t idx{};
	for (auto&& item : list) {
		lambda(idx++, item);
	}
}

void each_n(uint64_t offset, uint64_t group_size, auto list, auto lambda) {
	uint64_t idx{};
	for (auto&& item : list) {
		auto idx_n{ idx++ };
		if ((idx_n % group_size) == offset) {
			lambda(idx_n, item);
		}
	}
}

constexpr void constexpr_each_n(uint64_t offset, uint64_t group_size, auto list, auto lambda) {
	uint64_t idx{};
	for (auto&& item : list) {
		auto idx_n{ idx++ };
		if ((idx_n % group_size) == offset) {
			lambda(idx_n, item);
		}
	}
}