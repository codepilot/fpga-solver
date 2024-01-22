// typedef  struct {
//     uint  count;
//     uint  instanceCount;
//     uint  first;
//     uint  baseInstance;
// } DrawArraysIndirectCommand __attribute__ ((aligned(4))) __attribute__ ((packed));

/*
  nodes
    uint netIdx

  nets
    short2 sourceTile // source

*/

typedef struct {
    ushort2 sourceTile;
    ushort2 curTile;
} Stub __attribute__ ((aligned(4)));

constant ushort2 tileSize = (ushort2)(670, 311);
constant uint2 utileSize = (uint2)(670, 311);

uint tile_coords(ushort2 t) {
  uint2 tu = (uint2)(t.x, t.y);
  return (utileSize.x * tu.y) + tu.x;
}

float tile_distance(ushort2 a, ushort2 b) {
  return distance(convert_float2(a), convert_float2(b));
}

ushort2 best_next_tile(ushort2 sourcePos, ushort2 curPos, uint2 count_offset, constant ushort2 * restrict dest_tile) {
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
__attribute__((reqd_work_group_size(256, 1, 1)))
draw_wires(
  uint count,
  global ushort2 * restrict routed,
  global uint4 * restrict drawIndirect,
  global Stub * restrict stubLocations,
  constant uint2 * restrict tile_tile_count_offset,
  constant ushort2 * restrict dest_tile
) {
  if(drawIndirect[get_global_id(0)].w != 0) return; //dead end or success already

  global Stub * currentStub = stubLocations + get_global_id(0);

  ushort2 sourcePos = ((currentStub->sourceTile % tileSize) + tileSize) % tileSize;
  ushort2 curPos = ((currentStub->curTile % tileSize) + tileSize) % tileSize;

  if(sourcePos.x == curPos.x && sourcePos.y == curPos.y) {
    // finished successfully
    drawIndirect[get_global_id(0)].w = 1;
    return;
  }

  ushort2 newPos = best_next_tile(sourcePos, curPos, tile_tile_count_offset[tile_coords(curPos)], dest_tile);
  if(newPos.x == curPos.x && newPos.y == curPos.y) {
    // dead end
    drawIndirect[get_global_id(0)].w = 2;
    return;
  }

  currentStub->curTile = newPos;
  const uint first = get_global_id(0) * 1024;
  drawIndirect[get_global_id(0)] = (uint4)(count, 1, first, 0);
  routed[first + count] = newPos;

}
