@echo off
cmake -S . -B build --graphviz=build/fpga-solver.dot
cd build
dot -Tpng -o fpga-solver.png fpga-solver.dot
start fpga-solver.png
