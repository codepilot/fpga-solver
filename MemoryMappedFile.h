#pragma once

class MemoryMappedFile {
public:
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

  MemoryMappedFile(std::wstring fn) : fh{ CreateFile2(
    fn.c_str(),
    GENERIC_READ,
    or_reduce<DWORD>({FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE}),
    OPEN_EXISTING,
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

  MemoryMappedFile(std::wstring fn, uint64_t requestedSize) : fh{ CreateFile2(
    fn.c_str(),
    FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
    or_reduce<DWORD>({FILE_SHARE_READ, FILE_SHARE_WRITE, FILE_SHARE_DELETE}),
    CREATE_ALWAYS,
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
