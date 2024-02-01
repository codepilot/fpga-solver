cmake -S . -B build
cd build
cmake --build . --config Release -j -t opencl-node-router
Release\opencl-node-router.exe > ..\routing-results\mlcad_d181_lefttwo3rds.txt
call _deps\benchmark-files-build\mlcad_d181_lefttwo3rds.dcp.cmd >> ..\routing-results\mlcad_d181_lefttwo3rds.txt
call _deps\benchmark-files-build\mlcad_d181_lefttwo3rds.route_status.cmd
cd ..
