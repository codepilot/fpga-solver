#pragma once

template<typename T> constexpr T or_reduce(std::vector<T> v) {
    T ret{ 0 };
    for (auto& n : v) { ret |= n; }
    return ret;
};

class User32Class {
public:
    ATOM atom{ 0 };
    std::wstring wsClassName;
    User32Class(WNDCLASSEXW& classInfo) : atom{ RegisterClassExW(&classInfo) }, wsClassName{ classInfo.lpszClassName } {
    }
    ~User32Class() {
        UnregisterClassW(wsClassName.c_str(), GetModuleHandleW(nullptr));
    }
};

class GL46 {
public:

    static inline LRESULT Wndproc(const _In_ HWND   hwnd, const _In_ UINT   uMsg, const _In_ WPARAM wParam, const _In_ LPARAM lParam) {
        switch (uMsg)
        {
        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
            }
            break;
        }
        case WM_SIZE:
        {
            if (getThis(hwnd)) {
                getThis(hwnd)->clientWidth = LOWORD(lParam);
                getThis(hwnd)->clientHeight = HIWORD(lParam);
            }
            break;
        }
        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        case WM_CREATE:
        {
            CREATESTRUCTW* cs{ reinterpret_cast<CREATESTRUCTW*>(lParam) };
            GL46* that{ reinterpret_cast<GL46*>(cs->lpCreateParams) };
            SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(that));
            return 0;
        }
        default:
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        }
        return 0;
    }

    static inline WNDCLASSEXW classInfo{
        .cbSize{sizeof(WNDCLASSEXW)},
        .style{or_reduce<UINT>({
          CS_DROPSHADOW,
          CS_HREDRAW,
          CS_OWNDC,
          CS_VREDRAW
        })},
      .lpfnWndProc{Wndproc},
      .cbClsExtra{0},
      .cbWndExtra{sizeof(nullptr)},
      .hInstance{GetModuleHandleW(nullptr)},
      .hIcon{nullptr},
      .hCursor{nullptr},
      .hbrBackground{nullptr},
      .lpszMenuName{nullptr},
      .lpszClassName{L"GL46"},
      .hIconSm{nullptr},
    };
    static inline User32Class atom{ GL46::classInfo };
    static inline DWORD dwExStyle{ or_reduce<DWORD>({
      WS_EX_APPWINDOW,
      WS_EX_OVERLAPPEDWINDOW,
      WS_EX_LEFT,
      WS_EX_LTRREADING,
    }) };
    static inline DWORD dwStyle{ or_reduce<DWORD>({
      WS_CAPTION,
      WS_CLIPCHILDREN,
      WS_CLIPSIBLINGS,
      WS_OVERLAPPEDWINDOW,
      WS_VISIBLE,
    }) };

    UINT clientWidth{ 1 };
    UINT clientHeight{ 1 };

    HWND hwnd{};
    HDC hdc{};
    HGLRC hglrc{};
#include "declarations.h"

    static HWND make_window(GL46 *that) {
        return CreateWindowExW(
            dwExStyle,
            atom.wsClassName.c_str(),
            L"GL46",
            dwStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            that
        );
    }

    inline static constexpr BYTE color_channel_bits{ 8ui8 };
    static PIXELFORMATDESCRIPTOR make_pixel_format() {
        
        return {
              .nSize = sizeof(PIXELFORMATDESCRIPTOR),
              .nVersion = 1,
              .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
              .iPixelType = PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
              .cColorBits = 32,                   // Colordepth of the framebuffer.
              .cRedBits = color_channel_bits,
              .cRedShift = 0,
              .cGreenBits = color_channel_bits,
              .cGreenShift = 0,
              .cBlueBits = color_channel_bits,
              .cBlueShift = 0,
              .cAlphaBits = 0,
              .cAlphaShift = 0,
              .cAccumBits = 0,
              .cAccumRedBits = 0,
              .cAccumGreenBits = 0,
              .cAccumBlueBits = 0,
              .cAccumAlphaBits = 0,
              .cDepthBits = 24,                   // Number of bits for the depthbuffer
              .cStencilBits = 0,                    // Number of bits for the stencilbuffer
              .cAuxBuffers = 0,                    // Number of Aux buffers in the framebuffer.
              .iLayerType = PFD_MAIN_PLANE,
              .bReserved = 0,
              .dwLayerMask = 0,
              .dwVisibleMask = 0,
              .dwDamageMask = 0
        };
    }

    template<typename T> static inline T load(std::string procName) {
        T p = reinterpret_cast<T>(wglGetProcAddress(procName.c_str()));
        if (p == 0 ||
            (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
            (p == (void*)-1))
        {
            HMODULE module{ nullptr };
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, L"opengl32.dll", &module);
            p = reinterpret_cast<T>(GetProcAddress(module, procName.c_str()));
        }

        return p;
    }

    void loadAll() {
#define loadProc(n) {n = load<decltype(n)>(#n);}
#include "loadProcs.h"
#undef loadProc
    }

    std::unordered_set <std::string> extensions;

    void getExtensions(HDC hdc) {
        {
            GLint numExtensions{};
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
            for (GLint i = 0; i < numExtensions; i++) {
                std::string extI = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
                extensions.insert(extI);
                OutputDebugStringA((extI + "\r\n").c_str());
            }
        }
        {
            std::string ext{ wglGetExtensionsStringARB(hdc) };
            OutputDebugStringA((ext + "\r\n").c_str());
        }

    }

    void load_opengl() {
        {
            auto pfd{ make_pixel_format() };

            auto chosenFormat = ChoosePixelFormat(hdc, &pfd);
            SetPixelFormat(hdc, chosenFormat, &pfd);

            hglrc = wglCreateContext(hdc);
            wglMakeCurrent(hdc, hglrc);
            gl = this;

            loadAll();
            {
                std::vector<int> iAttribIList;
                std::vector<FLOAT> fAttribFList;
                std::vector<int> iFormats;
                UINT nNumFormats{ 0 };

                iAttribIList.push_back(WGL_DRAW_TO_WINDOW_ARB); iAttribIList.push_back(GL_TRUE);
                iAttribIList.push_back(WGL_SUPPORT_OPENGL_ARB); iAttribIList.push_back(GL_TRUE);
                iAttribIList.push_back(WGL_ACCELERATION_ARB); iAttribIList.push_back(WGL_FULL_ACCELERATION_ARB);
                //iAttribIList.push_back(WGL_SWAP_METHOD_ARB); iAttribIList.push_back(WGL_SWAP_EXCHANGE_EXT);

                //iAttribIList.push_back(WGL_COLOR_BITS_ARB); iAttribIList.push_back(30);
                iAttribIList.push_back(WGL_RED_BITS_ARB); iAttribIList.push_back(8/*10*/);

                iAttribIList.push_back(WGL_ALPHA_BITS_ARB); iAttribIList.push_back(0);
                iAttribIList.push_back(WGL_DEPTH_BITS_ARB); iAttribIList.push_back(24);
                iAttribIList.push_back(WGL_STENCIL_BITS_ARB); iAttribIList.push_back(0);
                iAttribIList.push_back(WGL_DOUBLE_BUFFER_ARB); iAttribIList.push_back(GL_TRUE);

                //iAttribIList.push_back(WGL_SAMPLE_BUFFERS_ARB); iAttribIList.push_back(GL_TRUE);
                //iAttribIList.push_back(WGL_SAMPLES_ARB); iAttribIList.push_back(16);
                //iAttribIList.push_back(WGL_COLORSPACE_EXT); iAttribIList.push_back(WGL_COLORSPACE_LINEAR_EXT);

                // iAttribIList.push_back(WGL_COLORSPACE_EXT); iAttribIList.push_back(WGL_COLORSPACE_SRGB_EXT);

                iAttribIList.push_back(0);

                fAttribFList.push_back(0);

                iFormats.resize(100);

                const auto results = wglChoosePixelFormatARB(
                    hdc,
                    iAttribIList.data(),
                    fAttribFList.data(),
                    SafeInt<UINT>(iFormats.size()),
                    iFormats.data(),
                    &nNumFormats
                );

                if (!results) {
                    DebugBreak();
                }

                iFormats.resize(nNumFormats);

                std::vector<int> iAttributes;

                iAttributes.push_back(WGL_DRAW_TO_WINDOW_ARB);
                iAttributes.push_back(WGL_ACCELERATION_ARB);
                iAttributes.push_back(WGL_SWAP_METHOD_ARB);
                iAttributes.push_back(WGL_TRANSPARENT_ARB);
                iAttributes.push_back(WGL_SUPPORT_OPENGL_ARB);
                iAttributes.push_back(WGL_DOUBLE_BUFFER_ARB);
                iAttributes.push_back(WGL_PIXEL_TYPE_ARB);
                iAttributes.push_back(WGL_COLOR_BITS_ARB);
                iAttributes.push_back(WGL_RED_BITS_ARB);
                iAttributes.push_back(WGL_GREEN_BITS_ARB);
                iAttributes.push_back(WGL_BLUE_BITS_ARB);
                iAttributes.push_back(WGL_ALPHA_BITS_ARB);
                iAttributes.push_back(WGL_DEPTH_BITS_ARB);
                iAttributes.push_back(WGL_STENCIL_BITS_ARB);
                iAttributes.push_back(WGL_SAMPLE_BUFFERS_ARB);
                iAttributes.push_back(WGL_SAMPLES_ARB);
                iAttributes.push_back(WGL_COLORSPACE_EXT);
                iAttributes.push_back(WGL_COVERAGE_SAMPLES_NV);
                iAttributes.push_back(WGL_COLOR_SAMPLES_NV);

                iAttributes.push_back(WGL_FLOAT_COMPONENTS_NV);
                iAttributes.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV);
                iAttributes.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV);
                iAttributes.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV);
                iAttributes.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV);

                for (const auto& iPixelFormat : iFormats) {
                    std::vector<int> iValues;
                    iValues.resize(iAttributes.size());

                    wglGetPixelFormatAttribivARB(
                        hdc,
                        iPixelFormat,
                        0,
                        SafeInt<UINT>(iAttributes.size()),
                        iAttributes.data(),
                        iValues.data()
                    );

                    std::wstring info;

                    if (iValues[11] != 0) continue;
                    if (iValues[13] != 0) continue;

                    info += L"format: " + std::to_wstring(iPixelFormat) + L"\r\n";
                    info += L"WGL_DRAW_TO_WINDOW_ARB:  " + std::to_wstring(iValues[0]) + L"\r\n";
                    info += L"WGL_ACCELERATION_ARB:    " + std::to_wstring(iValues[1]) + L"\r\n";
                    info += L"WGL_SWAP_METHOD_ARB:     " + std::to_wstring(iValues[2]) + L"\r\n";
                    info += L"WGL_TRANSPARENT_ARB:     " + std::to_wstring(iValues[3]) + L"\r\n";
                    info += L"WGL_SUPPORT_OPENGL_ARB:  " + std::to_wstring(iValues[4]) + L"\r\n";
                    info += L"WGL_DOUBLE_BUFFER_ARB:   " + std::to_wstring(iValues[5]) + L"\r\n";
                    info += L"WGL_PIXEL_TYPE_ARB:      " + std::to_wstring(iValues[6]) + L"\r\n";
                    info += L"WGL_COLOR_BITS_ARB:      " + std::to_wstring(iValues[7]) + L"\r\n";
                    info += L"WGL_RED_BITS_ARB:        " + std::to_wstring(iValues[8]) + L"\r\n";
                    info += L"WGL_GREEN_BITS_ARB:      " + std::to_wstring(iValues[9]) + L"\r\n";
                    info += L"WGL_BLUE_BITS_ARB:       " + std::to_wstring(iValues[10]) + L"\r\n";
                    info += L"WGL_ALPHA_BITS_ARB:      " + std::to_wstring(iValues[11]) + L"\r\n";
                    info += L"WGL_DEPTH_BITS_ARB:      " + std::to_wstring(iValues[12]) + L"\r\n";
                    info += L"WGL_STENCIL_BITS_ARB:    " + std::to_wstring(iValues[13]) + L"\r\n";
                    info += L"WGL_SAMPLE_BUFFERS_ARB:  " + std::to_wstring(iValues[14]) + L"\r\n";
                    info += L"WGL_SAMPLES_ARB:         " + std::to_wstring(iValues[15]) + L"\r\n";
                    info += L"WGL_COLORSPACE_EXT:      " + std::to_wstring(iValues[16]) + L"\r\n";
                    info += L"WGL_COVERAGE_SAMPLES_NV: " + std::to_wstring(iValues[17]) + L"\r\n";
                    info += L"WGL_COLOR_SAMPLES_NV:    " + std::to_wstring(iValues[18]) + L"\r\n";
                    info += L"WGL_FLOAT_COMPONENTS_NV: " + std::to_wstring(iValues[19]) + L"\r\n";
                    info += L"WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV:    " + std::to_wstring(iValues[20]) + L"\r\n";
                    info += L"WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV:   " + std::to_wstring(iValues[21]) + L"\r\n";
                    info += L"WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV:  " + std::to_wstring(iValues[22]) + L"\r\n";
                    info += L"WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV: " + std::to_wstring(iValues[23]) + L"\r\n";
                    info += L"\r\n";
                    info += L"\r\n";

                    OutputDebugStringW(info.c_str());
                }
            }
            {
                std::array<int, 15> attribList{
                  WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                  WGL_CONTEXT_MINOR_VERSION_ARB, 6,
                  WGL_CONTEXT_FLAGS_ARB,
                  or_reduce<int>({
        #if _DEBUG
                      WGL_CONTEXT_DEBUG_BIT_ARB,
        #endif
                      WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        #if _DEBUG
                      WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB,
        #endif
                      WGL_CONTEXT_RESET_ISOLATION_BIT_ARB,
                    }),

                    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        #if _DEBUG
                    WGL_CONTEXT_OPENGL_NO_ERROR_ARB, FALSE,
        #else
                    WGL_CONTEXT_OPENGL_NO_ERROR_ARB, TRUE,
        #endif
                    WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, WGL_NO_RESET_NOTIFICATION_ARB,
                    WGL_CONTEXT_RELEASE_BEHAVIOR_ARB, WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB,
                    0,
                };

                auto new_hglrc = wglCreateContextAttribsARB(hdc, nullptr, attribList.data());
                if (new_hglrc == nullptr) {
                    OutputDebugStringW(L"new_hglrc error\r\n");
                    DebugBreak();
                }
                wglMakeCurrent(hdc, new_hglrc);
                gl = this;
                wglDeleteContext(hglrc);
            }
            getExtensions(hdc);
            {
                glDebugMessageCallbackARB([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
                    std::string strOut{ message };
                    strOut += "\r\n";
                    if (severity == GL_DEBUG_SEVERITY_HIGH_ARB) {
                        OutputDebugStringA(strOut.c_str());
                        DebugBreak();
                    }
                    else {
                        OutputDebugStringA(strOut.c_str());
                    }
                    }, nullptr);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            }

        }

    }

	GL46(): hwnd{ make_window(this) }, hdc{ GetDC(hwnd) } {
        ShowWindow(hwnd, SW_MAXIMIZE);
        if (false) {
            SetFocus(hwnd);
            SetCapture(hwnd);
            ShowCursor(FALSE);
            RECT rcClip{};
            GetWindowRect(hwnd, &rcClip);
            ClipCursor(&rcClip);
        }
        load_opengl();
	}

    UINT step() {
        for (;;) {
            const auto avail = MsgWaitForMultipleObjectsEx(0, nullptr, 0, QS_ALLINPUT, or_reduce<DWORD>({ MWMO_ALERTABLE, MWMO_INPUTAVAILABLE }));
            if (WAIT_TIMEOUT == avail) return 1;
            {
                MSG msg{};
                HWND hWnd{};
                const auto bRet = PeekMessageW(&msg, hWnd, 0, 0, or_reduce<UINT>({ PM_REMOVE, PM_NOYIELD }));
                if (bRet == 0) return 0;
                if (bRet == -1) {
                    DebugBreak();
                }
                std::wstring ws;
                ws += std::to_wstring(msg.message);
                ws += L"\r\n";
                // OutputDebugStringW(ws.c_str());
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (msg.message == WM_QUIT) {
                    return msg.message;
                }
            }
        }
    }

    void loop() {
        while (step() != WM_QUIT) {
            draw();
        }
    }

    void draw() {
        if (!getThis(hwnd)) return;
        if (!hglrc) return;
        glViewport(0, 0, gl->clientWidth, gl->clientHeight);
        glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
        glClear(or_reduce<GLbitfield>({ GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT }));

        SwapBuffers(hdc);
    }

    static GL46* getThis(HWND hwnd) {
        return reinterpret_cast<GL46*>(GetWindowLongPtrW(hwnd, 0));
    }
};