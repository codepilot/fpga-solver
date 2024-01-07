template<typename T>
class MMF_Dense_Sets {
public:

    MemoryMappedFile mmf;
    std::span<uint32_t> count;
    std::span<uint32_t> header;
    std::span<T> body;

    MMF_Dense_Sets(std::string filename) :
        mmf{ filename },
        count{ mmf.get_span<uint32_t>(0, 2ull) },
        header{ mmf.get_span<uint32_t>(sizeof(uint64_t), count[0] * 2ull) },
        body{ mmf.get_span<T>(calc_header_size(count[0]), count[1])} { }

    size_t size() const noexcept {
        return count[0];
    }

    size_t get_offset(size_t b) {
        size_t offset{ header[b * 2ull + 1ull] };
        return offset;
    }
    std::span<T> operator[](size_t b) {
        size_t count{ header[b * 2ull] };
        size_t offset{ header[b * 2ull + 1ull] };
        return body.subspan(offset, count);
    }

    void test(std::vector<std::vector<T>>& src) {
        puts("test start");
        if (size() != src.size()) {
            puts("size() != src.size()");
            abort();
        }

        for (size_t i{}; i < size(); i++) {
            auto n{ operator[](i) };
            auto rn{ src[i] };
            if (n.size() != rn.size()) {
                puts(std::format("this[{}].size({}) != src[{}].size({})", i, n.size(), i, rn.size()).c_str());
                abort();
            }
            for (size_t j{}; j < n.size(); j++) {
                if (memcmp(&n[j], &rn[j], sizeof(T)) != 0) {
                    // puts(std::format("this[{}][{}]({}) != src[{}][{}]({})", i, j, n[j], i, j, rn[j]).c_str());
                    abort();
                }
            }
        }
        puts("test finish");
    }

    static size_t calc_header_size(size_t chunk_count) {
        size_t element_size{ sizeof(T) };
        size_t element_size_mask{ element_size - 1ull };
        size_t header{ sizeof(uint64_t) + chunk_count * (sizeof(uint64_t)) };
        size_t padding{ element_size - (header & element_size_mask) };
        return header + padding;
    }

    static size_t calc_header_size(std::vector<std::vector<T>>& src) {
        return calc_header_size(src.size());
    }

    static size_t calc_body_count(std::vector<std::vector<T>>& src) {
        size_t ret{};
        for(auto &&srcn: src) {
            ret += srcn.size();
        }
        return ret;
    }

    static size_t calc_body_size(std::vector<std::vector<T>>& src) {
        size_t body{ calc_body_count(src) };
        return body * sizeof(T);
    }

    static size_t calc_file_size(std::vector<std::vector<T>>& src) {
        size_t header{ calc_header_size(src) };
        size_t body{ calc_body_size(src) };
        return header + body;
    }

    static void make(std::string dst, std::vector<std::vector<T>>& src) noexcept {
        {
            auto file_size{ calc_file_size(src) };
            puts(std::format("file_size: {}", file_size).c_str());
            MemoryMappedFile mmf{ dst, file_size };
            auto count{ mmf.get_span<uint32_t>(0, 2ull) };
            puts(std::format("count.data: 0x{:x}, count.size: {}", reinterpret_cast<uintptr_t>(count.data()), count.size()).c_str());

            count[0] = src.size();
            count[1] = calc_body_count(src);
            auto header{ mmf.get_span<uint32_t>(sizeof(uint64_t), src.size() * 2ull) };
            auto body{ mmf.get_span<T>(calc_header_size(src), calc_body_count(src)) };
            size_t item_offset{};
            size_t item_index{};
            for (auto&& n : src) {
                header[item_index] = static_cast<uint32_t>(n.size());
                header[item_index + 1ull] = static_cast<uint32_t>(item_offset);
                std::copy(n.begin(), n.end(), body.begin() + item_offset);
                item_index += 2ull;
                item_offset += n.size();
            }
        }
        {
            MMF_Dense_Sets<T> mds{ dst };
            mds.test(src);
        }
    }
};

using MMF_Dense_Sets_u32 = MMF_Dense_Sets<uint32_t>;
using MMF_Dense_Sets_u64 = MMF_Dense_Sets<uint64_t>;
using MMF_Dense_Sets_4xu32 = MMF_Dense_Sets<std::array<uint32_t, 4>>;