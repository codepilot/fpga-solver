#pragma once

template<std::size_t char_count>
class constexpr_string {
public:
	char chars[char_count]{};
	consteval constexpr_string(const char(&str)[char_count]) {
		std::copy_n(str, char_count, chars);
	}
	consteval char operator[](std::size_t n) const {
		return chars[n];
	}
	consteval std::size_t size() const {
		return char_count - 1;
	}
	consteval const char* data() const {
		return chars;
	}
};

