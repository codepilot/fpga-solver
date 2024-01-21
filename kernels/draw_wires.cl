typedef struct {
  uint  count;
  uint  instanceCount;
  uint  firstIndex;
  int   baseVertex;
  uint  baseInstance;
} DrawElementsIndirectCommand __attribute__ ((aligned(4))) __attribute__ ((packed));

/*
  nodes
    uint netIdx

  nets
    short2 sourceTile // source

*/

typedef struct {
    ushort2 sourceTile;
    ushort2 curTile;
} Stub __attribute__ ((aligned(4))) __attribute__ ((packed));

constant ushort2 tileSize = (ushort2)(670, 311);
constant uint2 utileSize = (uint2)(670, 311);

uint tile_coords(ushort2 t) {
  uint2 tu = (uint2)(t.x, t.y);
  return (utileSize.x * tu.y) + tu.x;
}

float tile_distance(ushort2 a, ushort2 b) {
  return distance(convert_float2(a), convert_float2(b));
}

ushort2 best_next_tile(ushort2 sourcePos, ushort2 curPos, uint2 count_offset, global const ushort2 * restrict dest_tile) {
  uint count = count_offset.x;
  uint offset = count_offset.y;
  ushort2 best_dt = curPos;
  float best_dist = tile_distance(sourcePos, best_dt);

  for(uint i = 0; i < count; i++) {
    ushort2 dt = dest_tile[offset + i];
    float cur_dist = tile_distance(sourcePos, dt);
    if(cur_dist < best_dist) {
      best_dist = cur_dist;
      best_dt = dt;
    }
  }
  return best_dt;
}

kernel void
__attribute__((work_group_size_hint(256, 1, 1)))
//__attribute__((reqd_work_group_size(256, 1, 1)))
draw_wires(
  global uint * restrict routed,
  // global uint * restrict unrouted,
  // global uint * restrict stubs,
  global DrawElementsIndirectCommand * restrict drawIndirect,
  global Stub * restrict stubLocations,
  global const uint2 * restrict tile_tile_count_offset,
  global const ushort2 * restrict dest_tile
) {
  // { uint pos = atom_inc(&drawIndirect[0].count); routed[pos] = pos / 2; drawIndirect[0].instanceCount = 1; }
  // { uint pos = atom_inc(&drawIndirect[1].count); unrouted[pos] = pos * 2; drawIndirect[1].instanceCount = 1; }
  // { uint pos = atom_inc(&drawIndirect[2].count); stubs[pos] = pos * 3; drawIndirect[2].instanceCount = 1; }
  global Stub * currentStub = stubLocations + get_global_id(0);

  ushort2 sourcePos = ((currentStub->sourceTile % tileSize) + tileSize) % tileSize;
  ushort2 curPos = ((currentStub->curTile % tileSize) + tileSize) % tileSize;

  if(sourcePos.x != curPos.x || sourcePos.y != curPos.y) {
    // short2 delta = convert_short2(sourcePos) - convert_short2(curPos);
    // ushort2 newPos = convert_ushort2(convert_short2(curPos) + clamp(delta, (short2)(-1, -1), (short2)(1, 1)));
    ushort2 newPos = best_next_tile(sourcePos, curPos, tile_tile_count_offset[tile_coords(curPos)], dest_tile);
    currentStub->curTile = newPos;
    if(newPos.x != curPos.x || newPos.y != curPos.y) {
      // uint pos = drawIndirect[get_global_id(0)].count += 2;
      // uint pos = get_global_id(0) * 2;
      routed[drawIndirect[get_global_id(0)].count++] = tile_coords(newPos);
      drawIndirect[get_global_id(0)].instanceCount = 1;
      // drawIndirect[get_global_id(0)].count = get_global_size(0);
    }
  }

}
