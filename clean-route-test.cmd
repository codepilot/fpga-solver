rd /s /q build
cmake -S . -B build
cd build
cmake --build . --config Release -t route_benchmarks
cd ..
