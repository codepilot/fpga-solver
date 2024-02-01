#pragma once

#ifndef _WIN32
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <span>
#include <Trivial_Span.h>
#include <Windows.h>
#include "constexpr_intrinsics.h"
#endif

#include <cstdint>
#include <cstdio>
#include <format>
#include "Trivial_Span.h"

class MemoryMappedFile {
public:

  bool is_writable{};

  std::string file_name{};

#ifdef _WIN32
  using api_MapViewOfFile3 = decltype(MapViewOfFile3)*;
  using api_CreateFileMapping2 = decltype(CreateFileMapping2)*;
  static inline HMODULE getDllHandle(std::wstring dllName) {
    HMODULE ret{ nullptr };
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, dllName.c_str(), &ret);
    return ret;
  }

  inline static HMODULE lib_kernelbase{ getDllHandle(L"kernelbase.dll") };
  inline static api_MapViewOfFile3 MapViewOfFile3{ reinterpret_cast<api_MapViewOfFile3>(GetProcAddress(lib_kernelbase, "MapViewOfFile3")) };
  inline static api_CreateFileMapping2 CreateFileMapping2{ reinterpret_cast<api_CreateFileMapping2>(GetProcAddress(lib_kernelbase, "CreateFileMapping2")) };

  HANDLE fh{ INVALID_HANDLE_VALUE };
#else
  int fh{-1};
#endif

  bool sparse{};
  uint64_t fsize{};
#ifdef _WIN32
  HANDLE fm{ nullptr };
#else
  int fm{-1};
#endif
  void *fp{ nullptr };

  inline void zero() {
#ifdef _WIN32
    FILE_ZERO_DATA_INFORMATION InBuffer{ .FileOffset{}, .BeyondFinalZero{.QuadPart{ static_cast<LONGLONG>( fsize ) }} };
    DWORD lpBytesReturned{};
    auto success{ DeviceIoControl(
      fh,                         // handle to a file
      FSCTL_SET_ZERO_DATA,                         // dwIoControlCode
      (PFILE_ZERO_DATA_INFORMATION)&InBuffer,     // input buffer
      (DWORD)sizeof(InBuffer),                    // size of input buffer
      NULL,                                     // lpOutBuffer
      0,                                        // nOutBufferSize
      (LPDWORD)&lpBytesReturned,                // number of bytes returned
      (LPOVERLAPPED)nullptr               // OVERLAPPED structure
    ) };

    if (!success) {
      puts("failed FSCTL_SET_ZERO_DATA");
      abort();
    }
#elif 1
    if(fallocate64(fh, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, 0, fsize)) {
      puts(std::format("failed fallocate64 errno:{}", errno).c_str());
      abort();
    }
#else
    memset(fp, 0, fsize);
#endif
  }

  static inline size_t getFileSize(decltype(fh) fh) {
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION info{};
    GetFileInformationByHandle(fh, &info);
    ULARGE_INTEGER ret{};
    ret.LowPart = info.nFileSizeLow;
    ret.HighPart = info.nFileSizeHigh;
    return ret.QuadPart;
#else
    struct stat64 sb{};
    fstat64(fh, &sb);
    return sb.st_size;
#endif
  }

  static inline size_t setFileSize(decltype(fh) fh, uint64_t newSize) {
#ifdef _WIN32
    FILE_END_OF_FILE_INFO info{ .EndOfFile{.QuadPart{static_cast<LONGLONG>(newSize)}} };
    auto result{ SetFileInformationByHandle(fh, FileEndOfFileInfo, &info, sizeof(info)) };
#else
    if(ftruncate64(fh, newSize)) {
      puts(std::format("ftruncate64(fh:{}, newSize:{}) failed errno:{}", fh, newSize, errno).c_str());
      abort();
    }
#endif
    return newSize;
  }

  inline static decltype(fh) open_readonly_file_handle(std::string fn) {
#ifdef _WIN32
  return CreateFileA(
    fn.c_str(),
    GENERIC_READ,
    or_reduce<DWORD>({FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE}),
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    nullptr);
#else
    return open(fn.c_str(), O_RDONLY);
#endif
  }

  inline static decltype(fh) create_readwrite_file_handle(std::string fn) {
#ifdef _WIN32
  return CreateFileA(
    fn.c_str(),
    FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
    or_reduce<DWORD>({FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE}),
    nullptr,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr);
#else
    int fh{open(fn.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR)};
    if(fh == -1) {
      puts(std::format("open create failed errno:{}", errno).c_str());
      int fh2{open(fn.c_str(), O_TRUNC | O_RDWR)};
      if(fh2 == -1) {
        puts(std::format("open rw failed errno:{}", errno).c_str());
        abort();
      }
      return fh2;
    }
    return fh;
#endif
  }

  inline static decltype(fm) create_readonly_file_mapping(decltype(fh) fh, decltype(fsize) fsize) {
#ifdef _WIN32
    return CreateFileMapping2(fh, nullptr, FILE_MAP_READ, PAGE_READONLY, 0, 0, nullptr, nullptr, 0);
#else
    return -1;
#endif
  }

  inline static decltype(fm) create_readwrite_file_mapping(decltype(fh) fh, decltype(fsize) fsize) {
#ifdef _WIN32
    return CreateFileMapping2(fh, nullptr, FILE_MAP_WRITE, PAGE_READWRITE, 0, fsize, nullptr, nullptr, 0);
#else
    return -1;
#endif
  }

  inline static decltype(fp) mmap_readonly(decltype(fh) fh, decltype(fsize) fsize, decltype(fm) fm) {
#ifdef _WIN32
      auto VirtualAddress{ MapViewOfFile3(fm, GetCurrentProcess(), nullptr, 0, 0, 0, PAGE_READONLY, nullptr, 0) };
      if (VirtualAddress) {
          auto VirtualAddresses{ std::array<WIN32_MEMORY_RANGE_ENTRY, 1>{ WIN32_MEMORY_RANGE_ENTRY{.VirtualAddress{VirtualAddress}, .NumberOfBytes{fsize} } } };
          auto prefetch_success{ PrefetchVirtualMemory(GetCurrentProcess(), VirtualAddresses.size(), VirtualAddresses.data(), 0) };
          if (!prefetch_success) {
              puts(std::format("PrefetchVirtualMemory last error: {}\n", GetLastError()).c_str());
          }
      }
      return VirtualAddress;
#else
    return mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fh, 0);
#endif
  }

  inline static decltype(fp) mmap_readwrite(decltype(fh) fh, decltype(fsize) fsize, decltype(fm) fm) {
#ifdef _WIN32
    return MapViewOfFile3(fm, GetCurrentProcess(), nullptr, 0, 0, 0, PAGE_READWRITE, nullptr, 0);
#else
    return mmap(nullptr, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fh, 0);
#endif
  }

  inline static bool make_sparse(decltype(fh) fh) noexcept {
#ifdef _WIN32
    FILE_SET_SPARSE_BUFFER InBuffer{ .SetSparse{true} };
    DWORD lpBytesReturned{};
    auto result{ DeviceIoControl(
        fh,                         // handle to a file
        FSCTL_SET_SPARSE,                         // dwIoControlCode
        (PFILE_SET_SPARSE_BUFFER)&InBuffer,     // input buffer
        (DWORD)sizeof(InBuffer),                    // size of input buffer
        NULL,                                     // lpOutBuffer
        0,                                        // nOutBufferSize
        (LPDWORD)&lpBytesReturned,                // number of bytes returned
        (LPOVERLAPPED)nullptr               // OVERLAPPED structure
    ) };
#endif
    return true;
  }

  inline MemoryMappedFile() : file_name{ "<EMPTY>" } {
  }

  inline MemoryMappedFile(MemoryMappedFile&& other) noexcept :
      file_name{ std::move(other.file_name) },
      fh{ other.fh },
      sparse{ other.sparse },
      fsize{ other.fsize },
      fm{ other.fm },
      fp{ other.fp }
  {
      other.file_name = "<EMPTY>";
#ifdef _WIN32
      other.fh = INVALID_HANDLE_VALUE;
#else
      other.fh = 0;
#endif
      other.sparse = false;
      other.fsize = 0;
#ifdef _WIN32
      other.fm = nullptr;
#else
      other.fm = -1;
#endif
      other.fp = nullptr;
  }

  inline MemoryMappedFile(std::string fn):
    file_name{ fn },
    is_writable{ false },
    fh{open_readonly_file_handle(fn)},
    fsize{getFileSize(fh)},
    fm{create_readonly_file_mapping(fh, fsize)},
    fp{mmap_readonly(fh, fsize, fm)} {
#ifdef _DEBUG
      puts(std::format("fn:{} fh:{} fsize:{} fp:{}", fn, fh, fsize, fp).c_str());
#endif
  }

  inline MemoryMappedFile(decltype(fh) fh, bool is_writable = false) :
    file_name{ "<FROM HANDLE>" },
    is_writable{ is_writable },
    fh{fh},
    fsize{getFileSize(fh)},
    fm{ is_writable?create_readwrite_file_mapping(fh, fsize):create_readonly_file_mapping(fh, fsize)},
    fp{ is_writable?mmap_readwrite(fh, fsize, fm) : mmap_readonly(fh, fsize, fm)} {
#ifdef _DEBUG
      puts(std::format("fh:{} fsize:{} fp:{}", fh, fsize, fp).c_str());
#endif
  }

  inline MemoryMappedFile(std::string fn, uint64_t requestedSize):
    file_name{ fn },
    is_writable{ true },
    fh{ create_readwrite_file_handle(fn)},
    sparse{ make_sparse(fh) },
    fsize{setFileSize(fh, requestedSize) },
    fm{create_readwrite_file_mapping(fh, fsize)},
    fp{mmap_readwrite(fh, fsize, fm)} {
#ifdef _DEBUG
      puts(std::format("fn:{} requestedSize:{} fh:{} fsize:{} fp:{}", fn, requestedSize, fh, fsize, fp).c_str());
#endif
      zero();
  }

  template<typename T>
  inline std::span<T> get_span(size_t byte_offset, size_t element_count) const noexcept {
      return std::span<T>{ reinterpret_cast<T*>(reinterpret_cast<uint8_t *>(fp) + byte_offset), (fsize - byte_offset) / sizeof(T) }.subspan(0, element_count);
  }

  template<typename T>
  inline std::span<T> get_span(size_t byte_offset) const noexcept {
      return { reinterpret_cast<T*>(reinterpret_cast<uint8_t *>(fp) + byte_offset), (fsize - byte_offset) / sizeof(T) };
  }

  template<typename T>
  inline std::span<T> get_span() const noexcept {
      return { reinterpret_cast<T*>(fp), fsize / sizeof(T)};
  }

  template<typename T>
  inline Trivial_Span<T> get_trivial_span() const noexcept {
      return Trivial_Span<T>::make( reinterpret_cast<T*>(fp), fsize / sizeof(T) );
  }

  inline void invalidate_file_handle() {
#ifdef __GNUC__
    fh = -1;
#endif
#ifdef _WIN32
    fh = INVALID_HANDLE_VALUE;
#endif
  }

  inline void unmap() {
#ifdef __GNUC__
    if(fp) munmap(fp, fsize); fp = nullptr;
#endif
#ifdef _WIN32
    if(fp) UnmapViewOfFile2(GetCurrentProcess(), fp, 0); fp = nullptr;
    if(fm) CloseHandle(fm); fm = nullptr;
#endif
  }

  inline MemoryMappedFile shrink(int64_t EndOfFile) {
    unmap();
    setFileSize(fh, EndOfFile);
    fsize = 0;
    MemoryMappedFile ret{ fh, is_writable };
    ret.file_name = std::move(file_name);
    file_name = "<EMPTY>";
    invalidate_file_handle();
    return ret;
  }

  inline void reopen_delete() {
#ifdef __GNUC__
#endif
#ifdef _WIN32
      auto rfh{ ReOpenFile(fh, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_FLAG_DELETE_ON_CLOSE) };
      if (rfh == INVALID_HANDLE_VALUE) {
          ExitProcess(1);
      }
      CloseHandle(rfh);
#endif
  }

  inline void close_file() {
#ifdef _WIN32
    if (fh != INVALID_HANDLE_VALUE) CloseHandle(fh); fh = INVALID_HANDLE_VALUE;
#else
    if (fh != -1) close(fh); fh = -1;
#endif
  }

  inline ~MemoryMappedFile() {
    unmap();
    close_file();
  }

};
