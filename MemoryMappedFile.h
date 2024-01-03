#pragma once

#ifdef __GNUC__
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

class MemoryMappedFile {
public:

  int fh{-1};
  bool sparse{};
  size_t fsize{};
  int fm{};
  void *fp{ nullptr };

  inline void zero() {
    memset(fp, 0, fsize);
  }

  static inline size_t getFileSize(decltype(fh) fh) {
    struct stat64 sb{};
    fstat64(fh, &sb);
    return sb.st_size;
  }

  static inline size_t setFileSize(decltype(fh) fh, uint64_t newSize) {
    if(ftruncate64(fh, newSize)) {
      puts("failed to truncate");
      abort();
    }
    return newSize;
  }

  MemoryMappedFile(std::string fn):
    fh{open(fn.c_str(), O_RDONLY)},
    fsize{getFileSize(fh)},
    fp{mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fh, 0)} {
      puts(std::format("fn:{} fh:{} fsize:{} fp:{}", fn, fh, fsize, fp).c_str());
  }

  MemoryMappedFile(int fh) : fh{fh},
    fsize{getFileSize(fh)},
    fp{mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fh, 0)} {
      puts(std::format("fh:{} fsize:{} fp:{}", fh, fsize, fp).c_str());
  }

  MemoryMappedFile(std::string fn, uint64_t requestedSize):
    fh{open(fn.c_str(), O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR)},
    fsize{setFileSize(fh, requestedSize) },
    fp{mmap(nullptr, fsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fh, 0)} {
      puts(std::format("fn:{} requestedSize:{} fh:{} fsize:{} fp:{}", fn, requestedSize, fh, fsize, fp).c_str());
  }

  template<typename T>
  inline std::span<T> get_span(size_t byte_offset, size_t element_count) const noexcept {
      return std::span<T>{ reinterpret_cast<T*>(reinterpret_cast<uint8_t *>(fp) + byte_offset), (fsize - byte_offset) / sizeof(T) }.subspan(0, element_count);
  }

  template<typename T>
  inline std::span<T> get_span() const noexcept {
    return { reinterpret_cast<T*>(fp), fsize / sizeof(T)};
  }

  MemoryMappedFile shrink(int64_t EndOfFile) {
    munmap(fp, fsize); fp = nullptr;
    setFileSize(fh, EndOfFile);
    fsize = 0;
    MemoryMappedFile ret{ fh };
    fh = -1;
    return ret;
  }

  void reopen_delete() {
  }
};
#elif defined(_WIN32)
class MemoryMappedFile {
public:
  using api_MapViewOfFile3 = decltype(MapViewOfFile3)*;
  using api_CreateFileMapping2 = decltype(CreateFileMapping2)*;

  __forceinline void zero() {
#if 0
      memset(fp, 0, fsize);
#else
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
          DebugBreak();
      }

#endif
  }

  static inline HMODULE getDllHandle(std::wstring dllName) {
    HMODULE ret{ nullptr };
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, dllName.c_str(), &ret);
    return ret;
  }

  inline static HMODULE lib_kernelbase{ getDllHandle(L"kernelbase.dll") };
  inline static api_MapViewOfFile3 MapViewOfFile3{ reinterpret_cast<api_MapViewOfFile3>(GetProcAddress(lib_kernelbase, "MapViewOfFile3")) };
  inline static api_CreateFileMapping2 CreateFileMapping2{ reinterpret_cast<api_CreateFileMapping2>(GetProcAddress(lib_kernelbase, "CreateFileMapping2")) };

  static inline size_t getFileSize(HANDLE fh) {
    BY_HANDLE_FILE_INFORMATION info{};
    GetFileInformationByHandle(fh, &info);
    ULARGE_INTEGER ret{};
    ret.LowPart = info.nFileSizeLow;
    ret.HighPart = info.nFileSizeHigh;
    return ret.QuadPart;
  }

  HANDLE fh{ INVALID_HANDLE_VALUE };
  bool sparse{};
  size_t fsize{};
  HANDLE fm{ nullptr };
  PVOID fp{ nullptr };

  template<typename T>
  __forceinline std::span<T> get_span(size_t byte_offset, size_t element_count) const noexcept {
      return std::span<T>{ reinterpret_cast<T*>(reinterpret_cast<uint8_t *>(fp) + byte_offset), (fsize - byte_offset) / sizeof(T) }.subspan(0, element_count);
  }

  template<typename T>
  __forceinline std::span<T> get_span(size_t byte_offset) const noexcept {
      return { reinterpret_cast<T*>(reinterpret_cast<uint8_t *>(fp) + byte_offset), (fsize - byte_offset) / sizeof(T) };
  }

  template<typename T>
  __forceinline std::span<T> get_span() const noexcept {
      return { reinterpret_cast<T*>(fp), fsize / sizeof(T)};
  }

  template<typename T>
  __forceinline Trivial_Span<T> get_trivial_span() const noexcept {
      return Trivial_Span<T>::make( reinterpret_cast<T*>(fp), fsize / sizeof(T) );
  }

  MemoryMappedFile(HANDLE fh) : fh{fh},
      fsize{ getFileSize(fh) },
      fm{ CreateFileMapping2(fh, nullptr, FILE_MAP_READ, PAGE_READONLY, 0, 0, nullptr, nullptr, 0) },
      fp{ MapViewOfFile3(fm, GetCurrentProcess(), nullptr, 0, 0, 0, PAGE_READONLY, nullptr, 0) }
  {
      if (fh == INVALID_HANDLE_VALUE) DebugBreak();
      if (!fm) DebugBreak();
      if (!fp) DebugBreak();
  }

  MemoryMappedFile(std::string fn) : fh{ CreateFileA(
    fn.c_str(),
    GENERIC_READ,
    or_reduce<DWORD>({FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE}),
    nullptr,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
    ) },
    fsize{ getFileSize(fh) },
    fm{ CreateFileMapping2(fh, nullptr, FILE_MAP_READ, PAGE_READONLY, 0, 0, nullptr, nullptr, 0) },
    fp{ MapViewOfFile3(fm, GetCurrentProcess(), nullptr, 0, 0, 0, PAGE_READONLY, nullptr, 0) }
  {
      if (fh == INVALID_HANDLE_VALUE) DebugBreak();
      if (!fm) DebugBreak();
      if (!fp) DebugBreak();
  }


  static bool make_sparse(HANDLE fh) noexcept {
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

      return true;
  }

  MemoryMappedFile(std::string fn, uint64_t requestedSize) : fh{ CreateFileA(
    fn.c_str(),
    FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
    or_reduce<DWORD>({FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE}),
    nullptr,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    nullptr
    ) },
    sparse{ make_sparse(fh) },
    fsize{ requestedSize },
    fm{ CreateFileMapping2(fh, nullptr, FILE_MAP_WRITE, PAGE_READWRITE, 0, requestedSize, nullptr, nullptr, 0) },
    fp{ MapViewOfFile3(fm, GetCurrentProcess(), nullptr, 0, 0, 0, PAGE_READWRITE, nullptr, 0) }
  {
      if (fh == INVALID_HANDLE_VALUE) DebugBreak();
      if (!fm) DebugBreak();
      if (!fp) DebugBreak();
  }

  MemoryMappedFile shrink(int64_t EndOfFile) {
      UnmapViewOfFile2(GetCurrentProcess(), fp, 0); fp = nullptr;
      CloseHandle(fm); fm = nullptr;
      FILE_END_OF_FILE_INFO info{ .EndOfFile{.QuadPart{EndOfFile}} };
      auto result{ SetFileInformationByHandle(fh, FileEndOfFileInfo, &info, sizeof(info)) };
      fsize = 0;
      // std::print("result: {}\n", result);
      MemoryMappedFile ret{ fh };
      fh = INVALID_HANDLE_VALUE;
      return ret;
  }

  void reopen_delete() {
      auto rfh{ ReOpenFile(fh, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_FLAG_DELETE_ON_CLOSE) };
      if (rfh == INVALID_HANDLE_VALUE) {
          ExitProcess(1);
      }
      CloseHandle(rfh);
  }

  ~MemoryMappedFile() {
    UnmapViewOfFile2(GetCurrentProcess(), fp, 0);
    CloseHandle(fm);
    CloseHandle(fh);
  }
};
#endif