@echo off
cls

cmake --build ./OpenCL-Headers/build --target install  --config Release

cmake --build ./OpenCL-ICD-Loader/build --target install  --config Release

cmake --build ./capnproto/build --config Release

rd generated /s /q

md generated
cd generated

md capnp

cd capnp
md testdata
cd ..

set rw=..\fpga-interchange-schema\interchange
set src=..\capnproto\c++\src
set src=..\capnproto\c++\src
set jv=..\capnproto-java\compiler\src\main\schema
set cap=..\capnproto\build\c++\src\capnp\Release\capnp.exe
set cpp=..\capnproto\build\c++\src\capnp\Release\capnpc-c++.exe
set capt=..\capnproto\build\c++\src\capnp\Release\capnpc-capnp.exe

copy "%src%\capnp\testdata\*" capnp\testdata\*
copy "%src%\capnp\*.capnp" capnp\*
copy "%src%\capnp\*.capnp" *
copy "%rw%\References.capnp"
copy "%rw%\DeviceResources.capnp"
copy "%rw%\LogicalNetlist.capnp"
copy "%rw%\PhysicalNetlist.capnp"
copy "%jv%\capnp\*.capnp" capnp\*

"%cap%" compile --import-path="." -o"%cpp%" "References.capnp"
"%cap%" compile --import-path="." -o"%cpp%" "DeviceResources.capnp"
"%cap%" compile --import-path="." -o"%cpp%" "LogicalNetlist.capnp"
"%cap%" compile --import-path="." -o"%cpp%" "PhysicalNetlist.capnp"

cd ..
