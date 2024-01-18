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
    ushort2 sourceTile // source

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
  currentStub->curTile += (ushort2)(2,1);

  {
    uint pos = atomic_add(&drawIndirect[0].count, 2);
    routed[pos]     = ((((uint)tileSize.x) * (((uint)currentStub->sourceTile.y) % ((uint)tileSize.y)))) + ((((uint)currentStub->sourceTile.x) % ((uint)tileSize.x)));
    routed[pos + 1] = ((((uint)tileSize.x) * (((uint)currentStub->curTile.y) % ((uint)tileSize.y))))    + ((((uint)currentStub->curTile.x) % ((uint)tileSize.x)));
    drawIndirect[0].instanceCount = 1;
  }

}
