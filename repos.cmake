include_guard(GLOBAL)
include(ExternalProject)
include(FetchContent)

cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(REPO_VULKAN_SDK_TAG "vulkan-sdk-1.3.275.0")
set(REPO_OPENCL_SDK_TAG "v2023.12.14")

FetchContent_Declare(capnproto               GIT_REPOSITORY https://github.com/codepilot/capnproto.git                                                                    OVERRIDE_FIND_PACKAGE)
# FetchContent_Declare(zlib                  GIT_REPOSITORY https://github.com/madler/zlib.git                           GIT_TAG v1.2.11                                  OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(tclap                   GIT_REPOSITORY https://github.com/mirror/tclap.git                          GIT_TAG v1.2.5                                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(stb                     GIT_REPOSITORY https://github.com/nothings/stb.git                                                                           OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(whereami                GIT_REPOSITORY https://github.com/gpakosz/whereami.git                                                                       OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(OpenCL-Headers          GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers.git           GIT_TAG ${REPO_OPENCL_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(OpenCL                  GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-ICD-Loader.git        GIT_TAG ${REPO_OPENCL_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(OpenCL-SDK              GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-SDK.git               GIT_TAG ${REPO_OPENCL_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(SPIRV-Headers           GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git            GIT_TAG ${REPO_VULKAN_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(SPIRV-Tools             GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git              GIT_TAG ${REPO_VULKAN_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(glslang                 GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git                  GIT_TAG ${REPO_VULKAN_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(vulkan                  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git            GIT_TAG ${REPO_VULKAN_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(VulkanHeaders           GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git           GIT_TAG ${REPO_VULKAN_SDK_TAG}                   OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(libpng                  GIT_REPOSITORY https://github.com/glennrp/libpng.git                        GIT_TAG v1.6.40                                  OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(libvpl                  GIT_REPOSITORY https://github.com/intel/libvpl.git                          GIT_TAG v2023.4.0                                OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(OpenGL-Registry         GIT_REPOSITORY https://github.com/KhronosGroup/OpenGL-Registry.git          GIT_TAG 0ef89b84d3bb5880a6553231d9cc64b2abd525a7 OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(EGL-Registry            GIT_REPOSITORY https://github.com/KhronosGroup/EGL-Registry.git             GIT_TAG 5f2c71f311d6cb031562f2d61517383542ecade7 OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(fpga-interchange-schema GIT_REPOSITORY https://github.com/chipsalliance/fpga-interchange-schema.git GIT_TAG c985b4648e66414b250261c1ba4cbe45a2971b1c OVERRIDE_FIND_PACKAGE)
FetchContent_Declare(capnproto-java          GIT_REPOSITORY https://github.com/capnproto/capnproto-java.git              GIT_TAG ed9a67c5fcd46604a88593625a9e38496b83d3ab OVERRIDE_FIND_PACKAGE)

FetchContent_Declare(device-file             URL            https://github.com/Xilinx/fpga24_routing_contest/releases/latest/download/xcvu3p.device     DOWNLOAD_NO_EXTRACT true)
FetchContent_Declare(benchmark-files         URL            https://github.com/Xilinx/fpga24_routing_contest/releases/latest/download/benchmarks.tar.gz DOWNLOAD_NO_EXTRACT false)
