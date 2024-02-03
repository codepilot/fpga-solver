#pragma once

#include "interchange_types.h"
#include <capnp/serialize.h>
#include "MemoryMappedFile.h"
#include <iostream>
#include <fstream>
#include <string>
#include "interchange_types.h"
#include <format>
#include "each.h"
#include "Timer.h"
#include <utility>

template<typename T>
class InterchangeFlat {
public:
	MemoryMappedFile mmf_unzipped;
	std::span<capnp::word> span_words;
	kj::ArrayPtr<capnp::word> words;
	capnp::FlatArrayMessageReader famr;
	T::Reader root;

	InterchangeFlat(std::string fn) :
		mmf_unzipped{ fn },
		span_words{ mmf_unzipped.get_span<capnp::word>() },
		words{ span_words.data(), span_words.size() },
		famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
		root{ famr.getRoot<T>() }
	{ }

};

using DevFlat = InterchangeFlat<DeviceResources::Device>;
