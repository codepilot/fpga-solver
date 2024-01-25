#pragma once

#include <thread>

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


void jthread_each(auto list, auto lambda) {
	uint64_t group_size{ std::thread::hardware_concurrency() };
	std::vector<std::jthread> threads;
	threads.reserve(group_size);
	for (size_t thread_offset{}; thread_offset < group_size; thread_offset++) {
		threads.emplace_back([thread_offset, &group_size, &list, &lambda]() {
			each_n(thread_offset, group_size, list, lambda);
		});
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