// #define _DEBUG

#ifndef max_workgroup_size
#define max_workgroup_size 256
#endif

#ifndef count_of_pip_count_offset
#define count_of_pip_count_offset 1
#endif

#ifndef count_pip_tile_body
#define count_pip_tile_body 1
#endif

#define max_tile_count 5884u
#define tt_body_count 4293068u

#ifndef ocl_counter_max
#define ocl_counter_max 1024u
#endif

#ifndef netCountAligned
#define netCountAligned 128u
#endif

#ifndef beam_width
#define beam_width 16u
#endif

#ifndef MAX_TILE_INDEX
#define MAX_TILE_INDEX 670u * 311u
#endif

typedef ushort4 routed_lines_t[ocl_counter_max];
typedef uint4 beam_t[beam_width];
typedef uint2 history_t[ocl_counter_max];

#ifndef make_ushort2
#define make_ushort2(arg0, arg1) (ushort2)(arg0, arg1)
#endif

#ifndef make_ushort4
#define make_ushort4(arg0, arg1, arg2, arg3) (ushort4)(arg0, arg1, arg2, arg3)
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

float tile_distance(uint2 a, uint2 b) {
    return distance(convert_float2(a), convert_float2(b));
}

kernel void
__attribute__((work_group_size_hint(max_workgroup_size, 1, 1)))
__attribute__((reqd_work_group_size(max_workgroup_size, 1, 1)))
draw_wires(
/*0*/uint series_id,
/*1*/global routed_lines_t* restrict routed,
/*2*/global uint4* restrict drawIndirect, //count, instanceCount, first, baseInstance
/*3*/global beam_t* restrict heads, //cost height parent pip_idx
/*4*/global history_t* restrict explored, //pip_idx parent
/*5*/constant uint2* restrict pip_count_offset, // count offset
/*6*/constant uint4* restrict pip_tile_body, // x, y, node0_idx, node1_idx
/*7*/constant uint4* restrict stubs, // x, y, node_idx, net_idx
/*8*/global uint* restrict dirty
) {
    global uint4* restrict drawIndirectN = &drawIndirect[get_global_id(0)];
    if ((*drawIndirectN)[3] != 0) return; //dead end or success already

    const uint4 stub = stubs[get_global_id(0)]; // x, y, node_idx, net_idx
    const uint2 stub_xy = stub.xy;
    const uint stub_node_idx = stub[2];
//    global routed_lines_t* restrict routedN = &routed[get_global_id(0)];
    global beam_t* restrict head = &heads[get_global_id(0)];
    global history_t* restrict explore = &explored[get_global_id(0)];

    beam_t cur_heads;// cost height parent pip_idx

    __attribute__((opencl_unroll_hint(beam_width))) for (uint j = 0; j < beam_width; j++) { cur_heads[j] = (*head)[j]; }


    __attribute__((opencl_unroll_hint(1)))
    for(uint count_index = 0; count_index < ocl_counter_max; count_index++) {

        {

#ifdef _DEBUG
            printf("cur_heads[0]: %v4u\n", cur_heads[0]);
#endif
            if(cur_heads[0][0] == UINT_MAX && cur_heads[1][1] == UINT_MAX && cur_heads[1][2] == UINT_MAX && cur_heads[1][3] == UINT_MAX) {
                // dead end
                (*drawIndirectN) = make_uint4(count_index, 1, get_global_id(0) * ocl_counter_max, 2);
                // (*routedN)[count_index] = sourcePos;
                // printf("ULONG_MAX (%lu)\n", get_global_id(0));
                return;
            }

            uint parent_id = series_id * ocl_counter_max + count_index;

            uint parent_height = cur_heads[0][1];
            uint parent_pip_idx = cur_heads[0][3];


            if(parent_pip_idx >= count_pip_tile_body) {
                printf("parent_pip_idx:%u >= count_pip_tile_body:%u\n", parent_pip_idx, count_pip_tile_body);
                return;
            }
            uint4 parent_pip_info = pip_tile_body[parent_pip_idx];

            uint parent_node1_idx = parent_pip_info[3];
            if(parent_node1_idx >= count_of_pip_count_offset) {
                printf("parent_node1_idx:%u >= count_of_pip_count_offset:%u\n", parent_node1_idx, count_of_pip_count_offset);
                return;
            }

#ifdef _DEBUG
            printf("\nparent_id: %u, parent_height: %u, parent_pip_idx: %u, parent_pip_info: %v4u, parent_node1_idx: %u\n",
                parent_id, parent_height, parent_pip_idx, parent_pip_info, parent_node1_idx);
#endif

            uint2 count_offset = pip_count_offset[parent_node1_idx];

            uint pip_count = count_offset[0];
            uint pip_offset = count_offset[1];

            (*explore)[count_index] = make_uint2(parent_pip_idx, parent_id);

#ifdef _DEBUG
            printf("count_offset: %v2u, pip_count: %u, pip_offset: %u, count_index: %u\n", count_offset, pip_count, pip_offset, count_index);
#endif

            __attribute__((opencl_unroll_hint(beam_width - 1)))
            for (uint j = 0; j < (beam_width - 1); j++) {
                cur_heads[j] = cur_heads[j + 1];
#ifdef _DEBUG
                printf("cur_heads[%u]: %v4u", j, cur_heads[j]);
#endif
            }
            cur_heads[beam_width - 1] = make_uint4(UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX);
#ifdef _DEBUG
            printf("cur_heads[%u]: %v4u", beam_width - 1, cur_heads[beam_width - 1]);
#endif
            for (uint i = 0; i < pip_count; i++) {

#ifdef _DEBUG
                printf("pip_offset:%u pip_count:%u, i: %u\n", pip_offset, pip_count, i);
#endif
                uint pip_idx = pip_offset + i;
                if(pip_idx >= count_pip_tile_body) {
                    printf("pip_idx:%u >= count_pip_tile_body:%u\n", pip_idx, count_pip_tile_body);
                    return;
                }

                uint4 pip_info = pip_tile_body[pip_idx]; // x, y, node0_idx, node1_idx
#ifdef _DEBUG
                printf("pip_info: %v4u\n", pip_info);
#endif
                uint pip_node1_idx = pip_info[3];
                float node_diff = ldexp((float)abs((int)stub_node_idx - (int)pip_node1_idx), -28);
                float cur_dist = tile_distance(stub_xy, pip_info.xy);
                float item_cost = ((cur_dist * 8.0f)) + ((float)parent_height * 32.0f) + node_diff;

                uint4 item = make_uint4(as_uint(item_cost), parent_height + 1u, parent_id, pip_idx);//cost, height, (parent_id,my_id), pip_idx

#ifdef _DEBUG
                printf("pip_node1_idx: %u, node_diff: %f, cur_dist: %f, item_cost: %f\nitem: %v4u\n", pip_node1_idx, node_diff, cur_dist, item_cost, item);
#endif
                for (uint j = 0; j < 16; j++) {
                    bool is_less = as_ulong(cur_heads[j].xy) < as_ulong(item.xy);
                    uint4 temp_stub = cur_heads[j];
                    cur_heads[j] = is_less?temp_stub:item;
                    item = is_less?item:temp_stub;
#ifdef _DEBUG
                    printf("cur_heads[%u]: %v4u", j, cur_heads[j]);
#endif
                }
            }

        }

        if(cur_heads[0][0] == UINT_MAX && cur_heads[1][1] == UINT_MAX && cur_heads[1][2] == UINT_MAX && cur_heads[1][3] == UINT_MAX) {
            // dead end
            (*drawIndirectN) = make_uint4(count_index, 1, get_global_id(0) * ocl_counter_max, 2);
            // (*routedN)[count_index] = sourcePos;
            // printf("ULONG_MAX (%lu)\n", get_global_id(0));
            return;
        }

        uint4 front_head = cur_heads[0];
        uint front_head_pip_idx = front_head[3];
#ifdef _DEBUG
        printf("front_head: %v4u, front_head_pip_idx: %u\n", front_head, front_head_pip_idx);
#endif
        if(front_head_pip_idx >= count_pip_tile_body) {
            printf("front_head_pip_idx:%u >= count_pip_tile_body:%u\n", front_head_pip_idx, count_pip_tile_body);
            return;
        }
        uint4 front_pip_info = pip_tile_body[front_head_pip_idx];
        uint2 front_head_pos = front_pip_info.xy;
        uint front_node1_idx = front_pip_info[3];

        if (stub_node_idx == front_node1_idx) {
            (*drawIndirectN) = make_uint4(count_index + 1, 1, get_global_id(0) * ocl_counter_max, 1);
            return;
        }

        //(*routedN)[count_index] = newPos;
    }

    (*drawIndirectN) = make_uint4(ocl_counter_max, 1, get_global_id(0) * ocl_counter_max, 0);

    __attribute__((opencl_unroll_hint(beam_width))) for (uint j = 0; j < beam_width; j++) { (*head)[j] = cur_heads[j]; }

    atomic_inc(dirty);
}
