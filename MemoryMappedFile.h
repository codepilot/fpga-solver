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
  size_t fsize{};
  HANDLE fm{ nullptr };
  PVOID fp{ nullptr };
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

  }
  ~MemoryMappedFile() {
    CloseHandle(fm);
    CloseHandle(fh);
    UnmapViewOfFile2(GetCurrentProcess(), fp, 0);
  }
};
