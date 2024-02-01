cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cd build
cmake --build . -t route_benchmarks -j
cd ..
