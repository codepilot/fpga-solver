rm build -r -d -f
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build --graphviz=build/fpga-solver.dot
cd build
dot -Tpng -o fpga-solver.png fpga-solver.dot
cmake --build . -t route_benchmarks -j
cd ..
