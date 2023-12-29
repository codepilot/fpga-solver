#pragma once

template<class T>
class CanonReader {
public:
    std::span<capnp::word> span_words;
    kj::ArrayPtr<capnp::word> words;
    kj::ArrayPtr<const capnp::word> segments[1];
    capnp::SegmentArrayMessageReader message;
    T::Reader reader;

    FORCEINLINE CanonReader(MemoryMappedFile& mmf) :
        span_words{ mmf.get_span<capnp::word>() },
        words{ span_words.data(), span_words.size() },
        segments{ words },
        message{ segments, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX } },
        reader{ message.getRoot<T>() } {
    }
};


template<class T>
class FlatReader {
public:
    std::span<capnp::word> span_words;
    kj::ArrayPtr<capnp::word> words;
    capnp::FlatArrayMessageReader message;
    T::Reader reader;

    FORCEINLINE FlatReader(MemoryMappedFile& mmf) :
        span_words{ mmf.get_span<capnp::word>() },
        words{ span_words.data(), span_words.size() },
        message{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX } },
        reader{ message.getRoot<T>() } {
    }
};