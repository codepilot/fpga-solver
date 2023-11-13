cmake -D CMAKE_INSTALL_PREFIX=./OpenCL-Headers/install -S ./OpenCL-Headers -B ./OpenCL-Headers/build

cmake -D CMAKE_PREFIX_PATH=C:\Users\root\Desktop\fpga-solver\OpenCL-Headers\install -D CMAKE_INSTALL_PREFIX=./OpenCL-ICD-Loader/install -S ./OpenCL-ICD-Loader -B ./OpenCL-ICD-Loader/build 

cmake -D WITH_ZLIB=OFF -S ./capnproto -B ./capnproto/build
