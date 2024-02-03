#pragma once

#include <thread>
#include <functional>

template<typename I, typename L, typename N>
I each(L &list, std::function<void(I, N)> lambda) {
	I idx{0};
	for (N &&item : list) {
		lambda(idx, item);
		++idx;
	}
	return idx;
}

uint64_t each_n(uint64_t offset, uint64_t group_size, auto &&list, auto lambda) {
	uint64_t idx{0};
	for (auto&& item : list) {
		if ((idx % group_size) == offset) {
			lambda(idx, item);
		}
		++idx;
	}
	return idx;
}


uint64_t jthread_each(auto &&list, auto lambda) {
	uint64_t group_size{ std::thread::hardware_concurrency() };
	std::vector<std::jthread> threads;
	threads.reserve(group_size);
	for (size_t thread_offset{}; thread_offset < group_size; thread_offset++) {
		threads.emplace_back([thread_offset, &group_size, &list, &lambda]() {
			each_n(thread_offset, group_size, list, lambda);
		});
	}
	return group_size;
}

constexpr uint64_t constexpr_each_n(uint64_t offset, uint64_t group_size, auto &&list, auto lambda) {
	uint64_t idx{0};
	for (auto&& item : list) {
		if ((idx % group_size) == offset) {
			lambda(idx, item);
		}
		++idx;
	}
	return idx;
}