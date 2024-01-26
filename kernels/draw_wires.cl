constant uint max_tile_count = 2268;
constant uint tt_body_count = 970435;

#ifndef ocl_counter_max
#define ocl_counter_max 1024
#endif

typedef ushort2 routed_lines[ocl_counter_max];

#ifndef make_ushort2
#define make_ushort2(arg0, arg1) (ushort2)(arg0, arg1)
#endif

#ifndef make_uint2
#define make_uint2(arg0, arg1) (uint2)(arg0, arg1)
#endif

#ifndef make_uint3
#define make_uint3(arg0, arg1, arg2) (uint3)(arg0, arg1, arg2)
#endif

#ifndef make_uint4
#define make_uint4(arg0, arg1, arg2, arg3) (uint4)(arg0, arg1, arg2, arg3)
#endif

#ifndef make_ulong2
#define make_ulong2(arg0, arg1) (ulong2)(arg0, arg1)
#endif

constant ushort2 tileSize = make_ushort2(670, 311);
constant uint2 utileSize = make_uint2(670, 311);

uint3 split(ulong n) {
    return make_uint3(
        (n >> 51ull) & 0x01FFFull, // cost: 13 bit
        (n >> 39ull) & 0x00FFFull, // previous: 12 bit
        (n >> 19ull) & 0xFFFFFull  // tt_id:20 bit
        );
}

ulong combine(ulong cost, ulong previous, ulong tt_id, ulong tile_col, ulong tile_row) {
    return
        ((cost & 0x01FFFull) << 51ull) |
        ((previous & 0x00FFFull) << 39ull) |
        ((tt_id & 0xFFFFFull) << 19ull) |
        ((tile_row & 0x001FFull) << 10ull) |
        ((tile_col & 0x003FFull) << 0ull);
}

uint tile_coords(ushort2 t) {
    uint2 tu = make_uint2(t[0], t[1]);
    return (utileSize[0] * tu[1]) + tu[0];
}

ushort2 tile_to_coords(ulong t) {
    return make_ushort2(t & 0x3ffull, (t >> 10ull) & 0x1ffull);
}

float tile_distance(ushort2 a, ushort2 b) {
    return distance(convert_float2(a), convert_float2(b));
}

ushort2 best_next_tile(ushort2 sourcePos, constant uint2* restrict tile_tile_count_offset, constant ushort2* restrict dest_tile, global ulong16* restrict stubLocationN) {
    ulong16 curStubs = (*stubLocationN);
    ushort2 curPos = tile_to_coords(curStubs[0]);
    uint3 curInfo = split(curStubs[0]);
    uint previous = curInfo[1];
    uint2 count_offset = tile_tile_count_offset[tile_coords(curPos)];
    uint tt_count = count_offset[0];
    uint offset = count_offset[1];

    for (uint j = 0; j < 15; j++) {
        curStubs[j] = curStubs[j + 1];
    }
    curStubs[15] = 0xffffffffffffffff;

    for (uint i = 0; i < tt_count; i++) {
        ushort2 dt = dest_tile[offset + i];
        if (curPos[0] == dt[0] && curPos[1] == dt[1]) continue;

        float cur_dist = tile_distance(sourcePos, dt);
        ulong item = combine(((ulong)(cur_dist * 8.0f)) + previous * 32, previous + 1, 0, dt[0], dt[1]);//cost, previous, tt_id, tile
        for (uint j = 0; j < 16; j++) {
            if ((0x7FFFFull & curStubs[j]) == (0x7FFFFull & item)) break;
            ulong2 maybe_swap = make_ulong2(min(curStubs[j], item), max(curStubs[j], item));
            curStubs[j] = maybe_swap[0]; item = maybe_swap[1];
        }
    }

    ulong ret = curStubs[0];
    (*stubLocationN) = curStubs;

    return tile_to_coords(ret);
}

kernel void
__attribute__((work_group_size_hint(256, 1, 1)))
__attribute__((reqd_work_group_size(256, 1, 1)))
draw_wires(
    uint series_id,
    global routed_lines* restrict routed,
    global uint4* restrict drawIndirect,
    global ulong16* restrict stubLocations, // cost: 13 bit, previous: 12, tt_id:20bit, tile:19bit
    constant uint2* restrict tile_tile_count_offset,
    constant ushort2* restrict dest_tile,
    constant ushort2* restrict allSourcePos,
    global uint* restrict dirty
) {

    const ushort2 sourcePos = allSourcePos[get_global_id(0)];//s0.s01;
    global uint4* restrict drawIndirectN = drawIndirect + get_global_id(0);
    global routed_lines* restrict routedN = routed + get_global_id(0);
    global ulong16* restrict stubLocationN = stubLocations + get_global_id(0);

    __attribute__((opencl_unroll_hint(ocl_counter_max)))
    for(uint count_index = 0; count_index < ocl_counter_max; count_index++) {
        if ((*drawIndirectN)[3] != 0) return; //dead end or success already

        ushort2 curPos = tile_to_coords((*stubLocationN)[0]);

        if (sourcePos[0] == curPos[0] && sourcePos[1] == curPos[1]) {
            // finished successfully
            (*drawIndirectN)[3] = 1;
            return;
        }

        ushort2 newPos = best_next_tile(sourcePos, tile_tile_count_offset, dest_tile, stubLocationN);

        if (newPos[0] == curPos[0] && newPos[1] == curPos[1]) {
            // dead end
            (*drawIndirectN) = make_uint4(count_index + 1, 1, get_global_id(0) * ocl_counter_max, 2);
            (*routedN)[count_index] = sourcePos;
            return;
        }

        (*drawIndirectN) = make_uint4(count_index + 1, 1, get_global_id(0) * ocl_counter_max, 0);
        (*routedN)[count_index] = newPos;
    }
    atomic_inc(dirty);
}
