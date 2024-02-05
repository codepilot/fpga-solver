apt update
apt install -y wget
wget -qO- https://github.com/Kitware/CMake/releases/download/v3.28.2/cmake-3.28.2-linux-x86_64.tar.gz | tar xz --strip-components=1 -C /usr
cmake --help


export LLVM_VERSION=17
apt update
apt -y install software-properties-common
add-apt-repository -y 'deb [trusted=yes] http://apt.llvm.org/jammy/ llvm-toolchain-jammy main'
add-apt-repository -y 'deb [trusted=yes] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main'
add-apt-repository -y 'deb [trusted=yes] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main'
add-apt-repository -y 'deb-src [trusted=yes] http://apt.llvm.org/jammy/ llvm-toolchain-jammy main'
add-apt-repository -y 'deb-src [trusted=yes] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main'
add-apt-repository -y 'deb-src [trusted=yes] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main'
apt update
apt install -y wget python3-dev libpython3-dev build-essential ocl-icd-libopencl1 git pkg-config libclang-${LLVM_VERSION}-dev clang-${LLVM_VERSION} llvm-${LLVM_VERSION} make ninja-build ocl-icd-libopencl1 ocl-icd-dev ocl-icd-opencl-dev libhwloc-dev zlib1g zlib1g-dev clinfo dialog apt-utils libxml2-dev libclang-cpp${LLVM_VERSION}-dev libclang-cpp${LLVM_VERSION} llvm-${LLVM_VERSION}-dev
git clone https://github.com/pocl/pocl.git
cd pocl
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_ICD=TRUE -DCMAKE_INSTALL_PREFIX=/usr ..
make -j && make install
cd ..
cd ..


clinfo

apt-get -y update
apt-get -y install software-properties-common clinfo graphviz
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt -y update
apt-get -y update
apt-get -y install git build-essential wget python3
apt install zlib1g-dev gcc-13 g++-13 -y libc6 libtbb-dev
gcc -v
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 13 --slave /usr/bin/g++ g++ /usr/bin/g++-13
update-alternatives --set gcc /usr/bin/gcc-13
gcc -v
git clone https://github.com/codepilot/fpga-solver.git
cd fpga-solver
git submodule init
git submodule update
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build --graphviz=build/fpga-solver.dot
cd build
dot -Tpng -o fpga-solver.png fpga-solver.dot
cmake --build . -t route_benchmarks -j
cd ..
cd ..