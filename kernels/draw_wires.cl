constant uint max_tile_count = 2268;
constant uint tt_body_count = 970435;

typedef ushort2 routed_lines[1024];

constant ushort2 tileSize = (ushort2)(670, 311);
constant uint2 utileSize = (uint2)(670, 311);

uint4 split(ulong n) {
  return (uint4)(
    (n >> 51ull) & 0x01FFFull, // cost: 13 bit
    (n >> 39ull) & 0x00FFFull, // previous: 12 bit
    (n >> 19ull) & 0xFFFFFull, // tt_id:20 bit
               n & 0x7FFFFull  // tile:19 bit
  );
}

ulong combine(uint4 p) {//cost, previous, tt_id, tile
  return
    (((ulong)p[0] & 0x01FFFull) << 51ull) |
    (((ulong)p[1] & 0x00FFFull) << 39ull) |
    (((ulong)p[2] & 0xFFFFFull) << 19ull) |
    (((ulong)p[3] & 0x7FFFFull) <<  0ull);
}

uint tile_coords(ushort2 t) {
  uint2 tu = (uint2)(t.x, t.y);
  return (utileSize.x * tu.y) + tu.x;
}

ushort2 tile_to_coords(ulong t) {
  ulong tm = t & 0x7FFFFull;
  return (ushort2)(tm % tileSize.x, tm / tileSize.x);
}

float tile_distance(ushort2 a, ushort2 b) {
  return distance(convert_float2(a), convert_float2(b));
}

ushort2 best_next_tile(ushort2 sourcePos, constant uint2 * restrict tile_tile_count_offset, constant ushort2 * restrict dest_tile, global ulong16 * restrict stubLocations) {
  ulong16 curStubs = stubLocations[get_global_id(0)];
  ushort2 curPos = tile_to_coords(curStubs.s0);
  uint4 curInfo = split(curStubs.s0);
  uint previous = curInfo.s1;
  uint2 count_offset = tile_tile_count_offset[tile_coords(curPos)];
  uint count = count_offset.x;
  uint offset = count_offset.y;

  for(uint j = 0; j < 15; j++) {
    curStubs[j] = curStubs[j + 1];
  }
  curStubs[15] = 0xffffffffffffffff;

  for(uint i = 0; i < count; i++) {
    ushort2 dt = dest_tile[offset + i];
    if(curPos.x == dt.x && curPos.y == dt.y) continue;

    float cur_dist = tile_distance(sourcePos, dt);
    ulong16 current = curStubs;
    // ulong item = (((ulong)(cur_dist * 8.0)) << 51ull) | ((ulong)tile_coords(dt));
    ulong item = combine((uint4)(((ulong)(cur_dist * 8.0)) + previous, previous + 1, 0, tile_coords(dt)));//cost, previous, tt_id, tile
    for(uint j = 0; j < 16; j++) {
      if((0x7FFFFull & current[j]) == (0x7FFFFull & item)) break;
      curStubs[j] = min(current[j], item); item = max(current[j], item);
    }
  }

  ulong ret = curStubs[0];
  stubLocations[get_global_id(0)] = curStubs;

  return tile_to_coords(ret);
}

kernel void
__attribute__((work_group_size_hint(256, 1, 1)))
__attribute__((reqd_work_group_size(256, 1, 1)))
draw_wires(
  uint count,
  global routed_lines * restrict routed,
  global uint4 * restrict drawIndirect,
  global ulong16 * restrict stubLocations, // cost: 13 bit, previous: 12, tt_id:20bit, tile:19bit
  constant uint2 * restrict tile_tile_count_offset,
  constant ushort2 * restrict dest_tile,
  constant ushort2 * restrict allSourcePos
) {
  if(drawIndirect[get_global_id(0)].w != 0) return; //dead end or success already

  ushort2 sourcePos = allSourcePos[get_global_id(0)];//s0.s01;
  ushort2 curPos = tile_to_coords(stubLocations[get_global_id(0)].s0);

  if(sourcePos.x == curPos.x && sourcePos.y == curPos.y) {
    // finished successfully
    drawIndirect[get_global_id(0)].w = 1;
    return;
  }

  ushort2 newPos = best_next_tile(sourcePos, tile_tile_count_offset, dest_tile, stubLocations);
  if(newPos.x == curPos.x && newPos.y == curPos.y) {
    // dead end
    drawIndirect[get_global_id(0)] = (uint4)(count + 1, 1, get_global_id(0) * 1024, 2);
    routed[get_global_id(0)][count] = sourcePos;
    return;
  }

  drawIndirect[get_global_id(0)] = (uint4)(count + 1, 1, get_global_id(0) * 1024, 0);
  routed[get_global_id(0)][count] = newPos;

}
