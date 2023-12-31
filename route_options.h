#pragma once

class route_option {
public:
    __m128i v;
    __forceinline uint32_t get_wire0_idx() const noexcept { return v.m128i_u32[0]; }
    __forceinline uint32_t get_wire1_idx() const noexcept { return v.m128i_u32[1]; }
    __forceinline uint32_t get_previous() const noexcept { return v.m128i_u32[2]; }
    __forceinline uint16_t get_past_cost() const noexcept { return v.m128i_u16[6]; }
    __forceinline uint16_t get_future_cost() const noexcept { return v.m128i_u16[7]; }
    __forceinline uint32_t get_total_cost() const noexcept { return static_cast<uint32_t>(get_past_cost()) + static_cast<uint32_t>(get_future_cost()); }
};

class RouteComparison {
public:
    std::vector<route_option>& storage;
    RouteComparison(std::vector<route_option>& storage) : storage{ storage } {}
    bool operator() (uint32_t left, uint32_t right) {
        return storage[left].get_total_cost() > storage[right].get_total_cost();
    }
};

class route_options {
public:
  std::vector<route_option> storage;
  std::priority_queue<uint32_t, std::vector<uint32_t>, RouteComparison> q5;
  __forceinline uint32_t append(route_option ro) {
    uint32_t ret{ static_cast<uint32_t>(storage.size()) };
    storage.emplace_back(ro);
    q5.emplace(ret);
    return ret;
  }
  __forceinline uint32_t append(uint32_t wire0_idx, uint32_t wire1_idx, uint32_t previous, uint16_t past_cost, uint16_t future_cost) {
    __m128i v{};
    v.m128i_u32[0] = wire0_idx;
    v.m128i_u32[1] = wire1_idx;
    v.m128i_u32[2] = previous;
    v.m128i_u16[6] = past_cost;
    v.m128i_u16[7] = future_cost;

    return append({ .v{v} });
  }
  route_options() : q5{RouteComparison{ storage }} {
    // append(0, 0, 0, 0, 0xffff);
  }
};

