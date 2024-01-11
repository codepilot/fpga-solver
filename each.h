#pragma once

void each(auto list, auto lambda) {
	uint64_t idx{};
	for (auto&& item : list) {
		lambda(idx++, item);
	}
}