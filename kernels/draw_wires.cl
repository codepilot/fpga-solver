typedef struct  {
    uint  count;
    uint  instanceCount;
    uint  firstIndex;
    int   baseVertex;
    uint  baseInstance;
} DrawElementsIndirectCommand;

kernel void draw_wires(global int * bufA, global int * bufB, global int * bufC, global DrawElementsIndirectCommand * bufD) {
  uint pos = atom_inc(&bufD[0].count);
  bufA[pos] = pos;
  bufD[0].instanceCount = 1;
}
