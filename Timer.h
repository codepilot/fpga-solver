#pragma once

#include <chrono>
#include <iostream>
#include "constexpr_string.h"
#include <atomic>

static std::atomic<uint32_t> timer_depth{};

template<constexpr_string str>
class Timer {
public:
	const decltype(std::chrono::steady_clock::now()) start;
	Timer() : start{ std::chrono::steady_clock::now() } {
		timer_depth++;
	}
	std::chrono::duration<double> finish() {
		const auto end = std::chrono::steady_clock::now();
		const auto diff = end - start;
		return diff;
	}
	~Timer() {
		timer_depth--;
		std::cout << std::format("{:{}}", "", timer_depth.load() * 2) << std::format(str.chars, finish());
	}
	static auto log(auto lamda)-> decltype(lamda()) {
		auto execLambda{ Timer() };
		return lamda();
	}
};

#define StringOf2(arg) #arg
#define StringOf(arg) StringOf2(arg)
#define TimerVal(arg) Timer<("{:10} @ " __FILE__ ":" StringOf(__LINE__) " " #arg "\n")>::log([&]()-> decltype(arg) { return arg; })
