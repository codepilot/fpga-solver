#pragma once

class MMF_READONLY {
public:
    HANDLE fh{ INVALID_HANDLE_VALUE };
    size_t fileSize{ 0 };
    HANDLE fm{ INVALID_HANDLE_VALUE };
    LPVOID fp{ nullptr };
    MMF_READONLY(std::wstring_view fpath) : fh{ CreateFileW(
            std::wstring(fpath).c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr) },
        fileSize{ MMF_READONLY::file_size(fh) },
        fm{ CreateFileMappingW(fh, nullptr, PAGE_READONLY, 0, 0, nullptr) },
        fp{ MapViewOfFile(fm, FILE_MAP_READ, 0, 0, 0) } {

        CloseHandle(fm); fm = INVALID_HANDLE_VALUE;
        CloseHandle(fh); fh = INVALID_HANDLE_VALUE;
        std::array<WIN32_MEMORY_RANGE_ENTRY, 1> VirtualAddress{ {{.VirtualAddress{fp}, .NumberOfBytes{fileSize}} } };
        PrefetchVirtualMemory(GetCurrentProcess(), VirtualAddress.size(), VirtualAddress.data(), 0);
    }
    ~MMF_READONLY() {
        UnmapViewOfFile(fp);
        CloseHandle(fm); fm = INVALID_HANDLE_VALUE;
        CloseHandle(fh); fh = INVALID_HANDLE_VALUE;
    }
    static inline size_t file_size(HANDLE fh) {
        LARGE_INTEGER fileSize{};

        GetFileSizeEx(fh, &fileSize);
        return fileSize.QuadPart;
    }

};

template<typename T>
class MMF_READER {
public:
    kj::ArrayPtr<capnp::word> words;
    capnp::FlatArrayMessageReader famr;
    T::Reader reader;

    MMF_READER(MMF_READONLY& mmf) : words{
        reinterpret_cast<capnp::word*>(mmf.fp),
        mmf.fileSize / sizeof(capnp::word) },
        famr{ words, capnp::ReaderOptions{.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
        reader{ famr.getRoot<T>() } {

    }

};