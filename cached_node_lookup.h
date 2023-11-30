#pragma once

#include<type_traits>

class cached_node_lookup {
public:
    Trivial_Span<const __m128i> indirect;
    Trivial_Span_cu64 direct;
    Trivial_Span_cu64 direct_value;
    __forceinline static uint64_t pow2_roundup(uint64_t v) noexcept {
        return 1ui64 << (63ui64 - (_lzcnt_u64(v) - ((_mm_popcnt_u64(v) > 1i64) ? 1ui64 : 0ui64)));
    }

    static bool update_used_slots(std::vector<bool> temp_used_slots, std::span<uint64_t> idks) noexcept {
        const uint64_t used_slot_mask{ temp_used_slots.size() - 1ui64 };
        __m128i ko{};
        _rdseed64_step(ko.m128i_u64 + 0);
        _rdseed64_step(ko.m128i_u64 + 1);

        for (auto&& idk : idks) {
            const auto h0{ constexpr_siphash::make(ko, idk) };

            uint64_t slot{ used_slot_mask & idk };
            if (temp_used_slots[slot]) {
                return false;
            }
            temp_used_slots[slot] = true;
        }

        return true;
    }

    __declspec(noinline) static void make_cached_node_lookup(::capnp::List< ::DeviceResources::Device::Node, ::capnp::Kind::STRUCT>::Reader nodes, ::capnp::List< ::DeviceResources::Device::Wire, ::capnp::Kind::STRUCT>::Reader wires) noexcept {
        {
            uint64_t wire_count{ wires.size() };
            uint64_t wire_count_roundup{ pow2_roundup(wire_count) };
            uint64_t indirect_key_count{ wire_count_roundup >> 3ui64 };

            const uint64_t slot_count{ wire_count_roundup << 2ui64 };
            const uint64_t indirect_mask{ indirect_key_count - 1ui64 };
            const uint64_t used_slot_mask{ slot_count - 1ui64 };
            MemoryMappedFile mmf_indirect{ L"indirect.bin", indirect_key_count * sizeof(__m128i) };
            MemoryMappedFile mmf_direct{ L"direct.bin", slot_count * sizeof(uint64_t) };
            MemoryMappedFile mmf_direct_value{ L"direct_value.bin", slot_count * sizeof(uint64_t) };

            std::vector<bool> used_slots(slot_count, false);
            std::vector<std::vector<uint64_t>> indirect_keys(indirect_key_count);
            auto indirect_key_offsets{ mmf_indirect.get_span< __m128i>() };
            uint64_t indirect_key_offset_position{};
            auto slot_value{ mmf_direct.get_span<uint64_t>() };

            //tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.reserve(wires_size);
#if 0
            {
                uint32_t nodeIdx{};
                for (auto&& node : nodes) {
                    auto node_wires{ node.getWires() };
                    for (auto&& wireA_Idx : node_wires) {
                        auto wireA{ wires[wireA_Idx] };
                        auto wireA_tile_strIdx{ wireA.getTile() };
                        auto wireA_wire_strIdx{ wireA.getWire() };

                        ULARGE_INTEGER k{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                        ULARGE_INTEGER v{ .u{.LowPart{nodeIdx}, .HighPart{wireA_Idx}} };
                        //tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart);
                    }
                    nodeIdx++;
                }
            }
#endif
            {
                // uint32_t wireA_Idx{};
                for (auto&& wireA : wires) {
                    auto wireA_tile_strIdx{ wireA.getTile() };
                    auto wireA_wire_strIdx{ wireA.getWire() };

                    ULARGE_INTEGER k0{ .u{.LowPart{wireA_wire_strIdx}, .HighPart{wireA_tile_strIdx}} };
                    __m128i hk0{};
                    const auto h0{ constexpr_siphash::make(hk0, k0.QuadPart) };
                    uint64_t id{ h0.hash & indirect_mask };
                    indirect_keys[id].emplace_back(k0.QuadPart);
                    // ULARGE_INTEGER v{ .u{.LowPart{UINT32_MAX}, .HighPart{wireA_Idx}} };
                    // auto result{ tile_strIdx_high_wire_strIdx_low_to_wireIdx_high_nodeIdx_low.emplace(k.QuadPart, v.QuadPart) };
                    //wireA_Idx++;
                    }
                }
            uint64_t retry_count{};
            {
                uint64_t ko_index{};
                for (auto&& idks : indirect_keys) {
                    for (;;) {
                        if ([&]() -> bool {
                            __m128i ko{};
                            _rdseed64_step(ko.m128i_u64 + 0);
                            _rdseed64_step(ko.m128i_u64 + 1);

#if 0
                            std::vector<bool> temp_used_slots{ used_slots };

                            for (auto&& idk : idks) {
                                const auto h0{ constexpr_siphash::make(ko, idk) };

                                uint64_t slot{ used_slot_mask & h0.hash };
                                if (temp_used_slots[slot]) {
                                    OutputDebugStringA(std::format("indirect_key_offsets.size: {} retry\n", indirect_key_offsets.size()).c_str());
                                    return false;
                                }
                                temp_used_slots[slot] = true;
                            }
                            used_slots = temp_used_slots;
#else
                            std::vector<uint64_t> local_used_slots{};
                            local_used_slots.reserve(idks.size());
                            for (auto&& idk : idks) {
                                const auto h0{ constexpr_siphash::make(ko, idk) };

                                uint64_t slot{ used_slot_mask & h0.hash };
                                if (used_slots[slot] || std::ranges::count(local_used_slots, slot) > 0) {
                                    // OutputDebugStringA(std::format("indirect_key_offsets.size: {} retry\n", indirect_key_offsets.size()).c_str());
                                    retry_count++;
                                    return false;
                                }
                                local_used_slots.emplace_back(slot);
                            }
                            uint64_t slot_index{};
                            for (auto&& local_used_slot : local_used_slots) {
                                used_slots[local_used_slot] = true;
                                slot_value[local_used_slot] = idks[slot_index];
                                slot_index++;
                            }

#endif
                            indirect_key_offsets[indirect_key_offset_position] = ko;
                            indirect_key_offset_position++;
                            return true;
                        }()) break;
                    }
                    if ((indirect_key_offset_position % 100000) == 0)
                        OutputDebugStringA(std::format("indirect_key_offsets.size: {}, retry_count: {}, retry_ratio: {}\n", indirect_key_offset_position, retry_count, static_cast<double_t>(indirect_key_offset_position) / static_cast<double_t>(retry_count)).c_str());
                }
            }
            OutputDebugStringA(std::format("indirect_key_offsets.size: {}, retry_count: {}, retry_ratio: {}\n", indirect_key_offset_position, retry_count, static_cast<double_t>(indirect_key_offset_position) / static_cast<double_t>(retry_count)).c_str());
        }
        ExitProcess(0);
        //DebugBreak();


    }
    __forceinline static cached_node_lookup open_cached_node_lookup(MemoryMappedFile& mmf_indirect, MemoryMappedFile& mmf_direct, MemoryMappedFile& mmf_direct_value) noexcept {
        return {
            .indirect{mmf_indirect.get_trivial_span<const __m128i>()},
            .direct{mmf_direct.get_trivial_span<const uint64_t>()},
            .direct_value{mmf_direct_value.get_trivial_span<const uint64_t>()},
        };
    }
    DECLSPEC_NOINLINE __forceinline constexpr uint64_t at(uint32_t wire_strIdx, uint32_t tile_strIdx) const noexcept {
        const uint64_t indirect_mask{ indirect.size - 1ui64 };
        const uint64_t direct_mask{ direct.size - 1ui64 };
        const ULARGE_INTEGER k0{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
        __m128i hk0{};
        const auto h0{ constexpr_siphash::make(hk0, k0.QuadPart) };
        __m128i hk1{ indirect.data[h0.hash & indirect_mask] };
        const auto h1{ constexpr_siphash::make(hk1, k0.QuadPart) };

        const auto direct_index{ h1.hash & direct_mask };
        const auto found_key{ direct.data[h1.hash & direct_mask] };
        if (found_key == k0.QuadPart) {
            return direct_value.data[direct_index];
        }
        OutputDebugStringA(std::format("foundAt: {}, v:{}\n", found_key, k0.QuadPart).c_str());

        DebugBreak();
        std::unreachable();
    }
#if 0
    __forceinline constexpr void set_at(uint32_t wire_strIdx, uint32_t tile_strIdx, uint64_t v) noexcept {
        const uint64_t indirect_mask{ indirect.size - 1ui64 };
        const uint64_t direct_mask{ direct.size - 1ui64 };
        const ULARGE_INTEGER k0{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
        __m128i hk0{};
        const auto h0{ constexpr_siphash::make(hk0, k0.QuadPart) };
        __m128i hk1{ indirect.data[h0.hash & indirect_mask] };
        const auto h1{ constexpr_siphash::make(hk1, k0.QuadPart) };

        const auto direct_index{ h1.hash & direct_mask };
        const auto found_key{ direct.data[h1.hash & direct_mask] };
        if (found_key == k0.QuadPart) {
            direct_value.data[direct_index] = v;
            return;
        }
        OutputDebugStringA(std::format("foundAt: {}, v:{}\n", found_key, k0.QuadPart).c_str());

        DebugBreak();
        std::unreachable();

    }
#endif
};
static_assert(std::is_trivial_v<cached_node_lookup>&& std::is_standard_layout_v<cached_node_lookup>);

