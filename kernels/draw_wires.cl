typedef struct  {
  uint  count;
  uint  instanceCount;
  uint  firstIndex;
  int   baseVertex;
  uint  baseInstance;
} DrawElementsIndirectCommand __attribute__ ((aligned(4))) __attribute__ ((packed));

/*
  wires
    net idx
  nets
    source tile
  stub
    cur tile
    net idx
*/

kernel void draw_wires(global int * routed, global int * unrouted, global int * stubs, global DrawElementsIndirectCommand * drawIndirect) {
  { uint pos = atom_inc(&drawIndirect[0].count); routed[pos] = pos / 2; drawIndirect[0].instanceCount = 1; }
  { uint pos = atom_inc(&drawIndirect[1].count); unrouted[pos] = pos * 2; drawIndirect[1].instanceCount = 1; }
  { uint pos = atom_inc(&drawIndirect[2].count); stubs[pos] = pos * 3; drawIndirect[2].instanceCount = 1; }
}
