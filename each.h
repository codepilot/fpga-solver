#pragma once

#include <thread>
#include <functional>

template<typename I, typename L>
I each(L &list, std::function<void(I, decltype(*list.begin()))> lambda) {
	I idx{0};
	for (auto &&item : list) {
		lambda(idx, item);
		++idx;
	}
	return idx;
}

template<typename I, typename L>
I each_n(I offset, I group_size, L& list, std::function<void(I, decltype(*list.begin()))> lambda) {
	I idx{0};
	for (auto&& item : list) {
		if ((idx % group_size) == offset) {
			lambda(idx, item);
		}
		++idx;
	}
	return idx;
}


template<typename I, typename L>
I jthread_each(L& list, std::function<void(I, decltype(*list.begin()))> lambda) {
	I group_size{ static_cast<I>(std::thread::hardware_concurrency()) };
	std::vector<std::jthread> threads;
	threads.reserve(group_size);
	for (I thread_offset{0}; thread_offset < group_size; thread_offset++) {
		threads.emplace_back([thread_offset, &group_size, &list, &lambda]() {
			each_n<I, L>(thread_offset, group_size, list, lambda);
		});
	}
	return group_size;
}

template<typename I, typename L>
constexpr I constexpr_each_n(I offset, uint64_t group_size, L& list, std::function<void(I, decltype(*list.begin()))> lambda) {
	I idx{0};
	for (auto&& item : list) {
		if ((idx % group_size) == offset) {
			lambda(idx, item);
		}
		++idx;
	}
	return idx;
}