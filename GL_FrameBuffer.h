#pragma once

#include "gl46_base.h"

class GL_FrameBuffer {
public:
    GLuint id;
    ~GL_FrameBuffer() {
        OutputDebugStringW(L"~glDeleteFramebuffers()\r\n");
        GL46_Base::glDeleteFramebuffers(1, &id); id = 0;
    }
    static GL_FrameBuffer create() {
        GLuint id{};
        GL46_Base::glCreateFramebuffers(1, &id);
        OutputDebugStringW(L"glCreateFramebuffers\r\n");
        return GL_FrameBuffer{.id{id}};
    }
};