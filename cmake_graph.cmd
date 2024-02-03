@echo off
cmake -S . -B build --graphviz=build/fpga-solver.dot
cd build
dot -Tsvg -o fpga-solver.svg fpga-solver.dot
cd ..
