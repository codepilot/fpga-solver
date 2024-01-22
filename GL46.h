#pragma once

class User32Class {
public:
    ATOM atom{ 0 };
    std::string wsClassName;
    User32Class(WNDCLASSEXA& classInfo) : atom{ RegisterClassExA(&classInfo) }, wsClassName{ classInfo.lpszClassName } {
    }
    ~User32Class() {
        UnregisterClassA(wsClassName.c_str(), GetModuleHandleA(nullptr));
    }
};

#include "MemoryMappedFile.h"

#ifdef REBUILD_PHYS
#else
#define DRAW_ROUTED
#endif

#include "gl46_base.h"
#include "GL_Label.h"
#include "GL_Texture.h"
#include "GL_Buffer.h"
#include "GL_FrameBuffer.h"
#include "GL_VertexArray.h"
#include "GL_ProgramPipeline.h"
#include "GL_Shader.h"
#include "GL_Program.h"

typedef  struct {
    uint32_t  count;
    uint32_t  instanceCount;
    uint32_t  firstIndex;
    int32_t  baseVertex;
    uint32_t  baseInstance;
} DrawElementsIndirectCommand;
constexpr static inline size_t DrawElementsIndirectCommand_size{sizeof(DrawElementsIndirectCommand)};

typedef  struct {
    uint32_t  count;
    uint32_t  instanceCount;
    uint32_t  first;
    uint32_t  baseInstance;
} DrawArraysIndirectCommand;
constexpr static inline size_t DrawArraysIndirectCommand_size{ sizeof(DrawArraysIndirectCommand) };

#include "ocl_tile_router.h"
#include "Tile_Info.h"

class GL46 : public GL46_Base {
public:

    inline static GL46* getThis(HWND hwnd) {
        return reinterpret_cast<GL46*>(GetWindowLongPtrW(hwnd, 0));
    }

    inline static LRESULT Wndproc(const _In_ HWND   hwnd, const _In_ UINT   uMsg, const _In_ WPARAM wParam, const _In_ LPARAM lParam) {
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

    inline static WNDCLASSEXA classInfo{
        .cbSize{sizeof(WNDCLASSEXA)},
        .style{or_reduce<UINT>({
          CS_DROPSHADOW,
          CS_HREDRAW,
          CS_OWNDC,
          CS_VREDRAW,
        })},
        .lpfnWndProc{Wndproc},
        .cbClsExtra{0},
        .cbWndExtra{sizeof(nullptr)},
        .hInstance{GetModuleHandleW(nullptr)},
        .hIcon{nullptr},
        .hCursor{nullptr},
        .hbrBackground{nullptr},
        .lpszMenuName{nullptr},
        .lpszClassName{"GL46"},
        .hIconSm{nullptr},
    };

    inline static User32Class atom{ classInfo };

    inline static HWND make_window(GL46* that) {
        return CreateWindowExA(
            dwExStyle,
            atom.wsClassName.c_str(),
            "GL46",
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

    inline static constexpr GLsizei index_buf_bytes{ static_cast<GLsizei>((1ull * 1024ull * 1024ull) * sizeof(std::array<uint32_t, 2>)) };
    inline static constexpr size_t index_buf_lines{ static_cast<size_t>(index_buf_bytes / sizeof(std::array<uint32_t, 2>)) };

    inline static constexpr Tile_Info tileInfo{
        .minCol{0},
        .minRow{0},
        .maxCol{669},
        .maxRow{310},
        .numCol{670},
        .numRow{311},
    };

    UINT clientWidth;
    UINT clientHeight;

    HWND hwnd;
    HDC hdc;
    HGLRC hglrc;
    std::unordered_set <std::string> extensions;

    GL_Texture<GL_TEXTURE_RECTANGLE, "texture"> texture;
    GL_FrameBuffer<"fb"> fb;
    // std::vector<std::array<uint16_t, 2>> v_unrouted_locations;
    // std::span<std::array<uint16_t, 2>> unrouted_locations;

    // GL_Buffer<"vbo_locations"> vbo_locations;

    // GL_Buffer<"vio_unrouted"> vio_unrouted;
    // GL_Buffer<"vio_stubs"> vio_stubs;

    GL_VertexArray<"vaRouted"> vaRouted;
    // GL_VertexArray<"vaUnrouted"> vaUnrouted;
    // GL_VertexArray<"vaStubs"> vaStubs;

    PhysGZ phys;
    uint32_t netCount, netCountAligned, ocl_counter;
    constexpr inline static uint32_t ocl_counter_max{ 1024 };

    GL_Buffer<"vbo_routed"> vbo_routed;
    GL_Buffer<"vbo_status"> vbo_status;

//    std::vector<DrawArraysIndirectCommand> indirect_vec;
    GL_Buffer<"indirect_buf"> indirect_buf;

    OCL_Tile_Router ocltr;

    MemoryMappedFile vertex_spirv;
    MemoryMappedFile fragment_spirv;

    MemoryMappedFile vertex_glsl;
    MemoryMappedFile fragment_glsl;

    GL_Program<"vertex_program", GL_ShaderType::vertex> vertex_program;
    GL_Program<"fragment_program", GL_ShaderType::fragment> fragment_program;

    GL_ProgramPipeline<"program_pipeline"> program_pipeline;

    GL46() :
        clientWidth{ 1 },
        clientHeight{ 1 },
        hwnd{ make_window(this) },
        hdc{ GetDC(hwnd) },
        hglrc{ load_opengl(hdc) },
        extensions{ getExtensions(hdc) },
        fb{ },
        texture{ 1, GL_RGBA8, static_cast<GLsizei>(tileInfo.numCol), static_cast<GLsizei>(tileInfo.numRow) },
        // v_unrouted_locations{ tileInfo.get_tile_locations() },
        // unrouted_locations{ v_unrouted_locations },
        // vbo_locations{ static_cast<GLsizeiptr>(unrouted_locations.size_bytes()), unrouted_locations.data() },
        // vio_unrouted{ index_buf_bytes },
        // vio_stubs{ index_buf_bytes },
        vaRouted{ },
        // vaUnrouted{ },
        // vaStubs{ },
        phys{ "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" },
        netCount{ phys.root.getPhysNets().size() },
        netCountAligned{ (((netCount + 255ul) >> 8ul) << 8ul) },
        ocl_counter{},
        vbo_routed{ ocl_counter_max * netCountAligned * sizeof(std::array<uint16_t, 2>)},
        vbo_status{ std::array<std::array<float, 4>, 3>{
            std::array<float, 4>{0.0f, 1.0f, 0.0f, 0.01f}, // routing
            std::array<float, 4>{0.0f, 0.0f, 1.0f, 0.01f}, // success
            std::array<float, 4>{1.0f, 0.0f, 0.0f, 0.01f}, // failure
        } },
        // indirect_vec{ make_indirect(netCountAligned) },
        indirect_buf{ netCountAligned * sizeof(std::array<uint32_t, 4>) },
        ocltr{ OCL_Tile_Router::make(
            phys.root,
            {
                CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(ocl::platform::get_ids().value().at(0)),
                CL_GL_CONTEXT_KHR, reinterpret_cast<cl_context_properties>(hglrc),
                CL_WGL_HDC_KHR, reinterpret_cast<cl_context_properties>(hdc),
                0, 0,
            },
            {
                vbo_routed.id,
                indirect_buf.id
            }
        ) },
        vertex_spirv{ "shaders\\vertex.vert.spv" },
        fragment_spirv{ "shaders\\fragment.frag.spv" },
        vertex_glsl{ "..\\shaders\\vertex.vert" },
        fragment_glsl{ "..\\shaders\\fragment.frag" },
#if 0
        vertex_program{ GL_Program<"vertex_program", GL_ShaderType::vertex>::spirv_span(vertex_spirv.get_span<unsigned char>()) },
        fragment_program{ GL_Program<"fragment_program", GL_ShaderType::fragment>::spirv_span(fragment_spirv.get_span<unsigned char>()) },
#else
        vertex_program{ GL_Program<"vertex_program", GL_ShaderType::vertex>::glsl(vertex_glsl.get_span<char>().data()) },
        fragment_program{ GL_Program<"fragment_program", GL_ShaderType::fragment>::glsl(fragment_glsl.get_span<char>().data()) },
#endif
        program_pipeline{ }
    {
        ShowWindow(hwnd, SW_MAXIMIZE);
        if (false) {
            SetFocus(hwnd);
            SetCapture(hwnd);
            ShowCursor(FALSE);
            RECT rcClip{};
            GetWindowRect(hwnd, &rcClip);
            ClipCursor(&rcClip);
        }

        // phys.build();
        texture.clear(0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        fb.texture(GL_COLOR_ATTACHMENT0, texture, 0);

        // glTextureSubImage2D(textures[0], 0, 0, 0, static_cast<GLsizei>(tileInfo.numCol), static_cast<GLsizei>(tileInfo.numRow), GL_RGBA, GL_UNSIGNED_BYTE, dev.sp_tile_drawing.data());

        auto vab_routed{ vaRouted.attribBinding(0, 0) };
        vab_routed.bindingDivisor(0);
        vab_routed.attribFormat(2, GL_UNSIGNED_SHORT, GL_FALSE, 0);
        vab_routed.attribBuffer(vbo_routed, 0, 4);
        vab_routed.attribEnable();

        auto vab_status{ vaRouted.attribBinding(1, 1) };
        vab_status.bindingDivisor(1);
        vab_status.attribFormat(4, GL_FLOAT, GL_FALSE, 0);
        vab_status.attribBuffer(vbo_status, 0, 16);
        vab_status.attribEnable();

        //vaUnrouted.attribBinding(0, 0);
        //vaUnrouted.attribFormat(0, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0);
        //vaUnrouted.attribBuffer(0, vbo_locations, 0, 4);
        //vaUnrouted.attribEnable(0);
        //vaUnrouted.elementBuffer(vio_unrouted);

        //vaStubs.attribBinding(0, 0);
        //vaStubs.attribFormat(0, 2, GL_UNSIGNED_SHORT, GL_FALSE, 0);
        //vaStubs.attribBuffer(0, vbo_locations, 0, 4);
        //vaStubs.attribEnable(0);
        //vaStubs.elementBuffer(vio_stubs);

        std::span<uint32_t> mRouted{};
#if 0
        rp.start_routing(
            std::span<uint32_t>{reinterpret_cast<uint32_t*>(glMapNamedBufferRange(vio_routed, 0, index_buf_bytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)), index_buf_lines },
            std::span<uint32_t>{reinterpret_cast<uint32_t*>(glMapNamedBufferRange(vio_unrouted, 0, index_buf_bytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)), index_buf_lines },
            std::span<uint32_t>{reinterpret_cast<uint32_t*>(glMapNamedBufferRange(vio_stubs, 0, index_buf_bytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)), index_buf_lines }
        );
#endif
        wglSwapIntervalEXT(0);

#if 0
        MemoryMappedFile vertexGlsl{ L"../shaders/vertex.vert" };
        MemoryMappedFile fragmentGlsl{ L"../shaders/fragment.frag" };
        vertexShader = createShader(ShaderType::vertex, std::string{ reinterpret_cast<char*>(vertexGlsl.fp), vertexGlsl.fsize });
        fragmentShader = createShader(ShaderType::fragment, std::string{ reinterpret_cast<char*>(fragmentGlsl.fp), fragmentGlsl.fsize });
#endif
        program_pipeline.use(GL_VERTEX_SHADER_BIT, vertex_program);
        program_pipeline.use(GL_FRAGMENT_SHADER_BIT, fragment_program);

        program_pipeline.validate();
        vertex_program.uniform2f(0, static_cast<GLfloat>(tileInfo.numCol), static_cast<GLfloat>(tileInfo.numRow));

#if 1
        ;
#else
        auto sp_routed{ std::span<uint32_t>{reinterpret_cast<uint32_t*>(glMapNamedBufferRange(vio_routed, 0, index_buf_bytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)), index_buf_lines } };
        auto sp_unrouted{ std::span<uint32_t>{reinterpret_cast<uint32_t*>(glMapNamedBufferRange(vio_unrouted, 0, index_buf_bytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)), index_buf_lines } };
        auto sp_stubs{ std::span<uint32_t>{reinterpret_cast<uint32_t*>(glMapNamedBufferRange(vio_stubs, 0, index_buf_bytes, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT)), index_buf_lines } };

        auto d_routed{ sp_routed.data() };
        auto d_unrouted{ sp_unrouted.data() };
        auto d_stubs{ sp_stubs.data() };

        for (uint32_t n{}; n < 300 * 300; n++) {
            d_routed[draw_commands[0].count] = n;
            d_routed[draw_commands[0].count + 1] = n + 1;
            draw_commands[0].count += 2;
            draw_commands[0].instanceCount = 1;
        }
#endif

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

    LARGE_INTEGER time_step_freq{};
    LARGE_INTEGER prev_time_step{};
    std::array<int64_t, 1024> time_steps{};
    int64_t time_step_count{};
    int64_t time_step_sum{};
    void loop() {
        QueryPerformanceFrequency(&time_step_freq);
        QueryPerformanceCounter(&prev_time_step);
        while (step() != WM_QUIT) {
            draw();
        }
        OutputDebugStringA("WM_QUIT\r\n");
    }

    bool first_capture_png{ true };
    __declspec(noinline) void draw() {
        if (!getThis(hwnd)) return;
        if (!hglrc) return;
        glViewport(0, 0, clientWidth, clientHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(or_reduce<GLbitfield>({ GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, GL_STENCIL_BUFFER_BIT }));

        fb.blitDefault(0, 0, static_cast<GLsizei>(tileInfo.numCol), static_cast<GLsizei>(tileInfo.numRow),
            0, 0, static_cast<GLint>(clientWidth), static_cast<GLint>(clientHeight),
            GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (ocl_counter < ocl_counter_max) {
            ocltr.gl_step(ocl_counter).value();
            ocl_counter++;
        }

        indirect_buf.bind(GL_DRAW_INDIRECT_BUFFER, [&]() {
            program_pipeline.bind([&]() {
                vaRouted.bind([&]() {
                    // fragment_program.uniform4f(0, 0.0f, 1.0f, 0.0f, 0.1f);
                    glMultiDrawArraysIndirect(GL_LINE_STRIP, nullptr, netCount, 0);
                    //glMultiDrawElementsIndirect(GL_LINE_STRIP, GL_UNSIGNED_INT, reinterpret_cast<const void*>(DrawElementsIndirectCommand_size * 0), netCount, DrawElementsIndirectCommand_size);
                });

                //vaStubs.bind([&]() {
                //    fragment_program.uniform4f(0, 0.0f, 0.0f, 1.0f, 1.0f);
                //    glDrawElementsIndirect(GL_LINES, GL_UNSIGNED_INT, reinterpret_cast<const void*>(DrawElementsIndirectCommand_size * 1));
                //});

                //vaUnrouted.bind([&]() {
                //    fragment_program.uniform4f(0, 1.0f, 0.0f, 0.0f, 1.0f);
                //    glDrawElementsIndirect(GL_LINES, GL_UNSIGNED_INT, reinterpret_cast<const void*>(DrawElementsIndirectCommand_size * 2));
                //});
            });
        });

        // glInvalidateBufferData(vio_routed);
        // glInvalidateBufferData(vio_unrouted);
        // glInvalidateBufferData(vio_stubs);
        // glClearNamedBufferData(indirect_buf_id, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, nullptr);
        // glInvalidateBufferData(indirect_buf.id);
        // glClearNamedBufferData(indirect_buf.id, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, nullptr);

        glDisable(GL_BLEND);

        if (!first_capture_png) {
            first_capture_png = true;
            std::vector<uint8_t> src{ std::vector<uint8_t>(static_cast<size_t>(clientWidth) * static_cast<size_t>(clientHeight) * static_cast<size_t>(4), 0) };
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
            glClear(or_reduce<GLbitfield>({ GL_COLOR_BUFFER_BIT }));
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glReadnPixels(0,0, clientWidth, clientHeight, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(src.size()), src.data());
            size_t line_bytes{ 4 * static_cast<size_t>(clientWidth) };
            for (int line = 0; line != clientHeight / 2; ++line) {
                std::swap_ranges(
                    src.begin() + line_bytes * line,
                    src.begin() + line_bytes * (line + 1),
                    src.begin() + line_bytes * (static_cast<size_t>(clientHeight) - line - 1));
            }
            MemoryMappedFile mmf_dst{ "dst.png", static_cast<size_t>(clientWidth) * static_cast<size_t>(clientHeight) * static_cast<size_t>(4) + static_cast<size_t>(65536) };
            png_image img{
                .opaque{nullptr},
                .version{PNG_IMAGE_VERSION},
                .width{clientWidth},
                .height{clientHeight},
                .format{PNG_FORMAT_RGBA},
                .flags{},
                .colormap_entries{},
            };
            size_t memory_bytes{ mmf_dst.fsize };
            auto result{ png_image_write_to_memory(
                &img,
                mmf_dst.fp,
                &memory_bytes,
                0,
                src.data(),
                0,
                nullptr
            ) };
            OutputDebugStringW(std::format(L"png result: {}\n", result).c_str());
            mmf_dst.shrink(memory_bytes);
        }

        SwapBuffers(hdc);
        auto prev_ts{ prev_time_step.QuadPart };
        QueryPerformanceCounter(&prev_time_step);
        auto diff_ts{ prev_time_step.QuadPart - prev_ts };
        auto step_num{ (time_step_count++) & 1023i64 };
        time_step_sum += diff_ts - time_steps[step_num];
        time_steps[step_num] = diff_ts;
        double_t divisor{ time_step_count>=1024?1024.0: time_step_count };
        double_t fps{1.0 / (static_cast<double_t>(time_step_sum) / static_cast<double_t>(time_step_freq.QuadPart) / divisor)};
        auto title_text{ std::format(L"{:03.2f} fps, step: {}", fps, ocl_counter) };
        SetWindowTextW(hwnd, title_text.c_str());
    }

};