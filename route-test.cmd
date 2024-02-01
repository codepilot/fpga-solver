cmake -S . -B build
cd build
cmake --build . --config Release -j -t route_benchmarks
cd ..
