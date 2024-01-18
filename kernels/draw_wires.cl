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

kernel void draw_wires(global int * routed, global int * unrouted, global int * stubs, global DrawElementsIndirectCommand * drawIndirect, global Stub * stubLocations) {
  // { uint pos = atom_inc(&drawIndirect[0].count); routed[pos] = pos / 2; drawIndirect[0].instanceCount = 1; }
  // { uint pos = atom_inc(&drawIndirect[1].count); unrouted[pos] = pos * 2; drawIndirect[1].instanceCount = 1; }
  // { uint pos = atom_inc(&drawIndirect[2].count); stubs[pos] = pos * 3; drawIndirect[2].instanceCount = 1; }
  global Stub * currentStub = stubLocations + get_global_id(0);

  ushort2 sourcePos = ((currentStub->sourceTile % tileSize) + tileSize) % tileSize;
  ushort2 curPos = ((currentStub->curTile % tileSize) + tileSize) % tileSize;

  short2 delta = convert_short2(sourcePos) - convert_short2(curPos);
  ushort2 newPos = convert_ushort2(convert_short2(curPos) + clamp(delta, (short2)(-1, -1), (short2)(1, 1)));
  currentStub->curTile = newPos;

  if(sourcePos.x != curPos.x || sourcePos.y != curPos.y) {
    uint pos = atomic_add(&drawIndirect[0].count, 2);
    // uint pos = get_global_id(0) * 2;
    routed[pos]     = ((((uint)tileSize.x) * (((uint)newPos.y)))) + ((((uint)newPos.x)));
    routed[pos + 1] = ((((uint)tileSize.x) * (((uint)curPos.y)))) + ((((uint)curPos.x)));
    drawIndirect[0].instanceCount = 1;
    // drawIndirect[0].count = get_global_size(0);
  }

}
