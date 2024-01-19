#pragma once

#include "gl46_base.h"

class GL_VertexArray {
public:
    GLuint id;
    ~GL_VertexArray() {
        OutputDebugStringW(L"~glDeleteVertexArrays()\r\n");
        GL46_Base::glDeleteVertexArrays(1, &id); id = 0;
    }
    static GL_VertexArray create() {
        GLuint id{};
        GL46_Base::glCreateVertexArrays(1, &id);
        OutputDebugStringW(L"glCreateVertexArrays\r\n");
        return GL_VertexArray{.id{id}};
    }
    void bind(auto lambda) const {
      GL46_Base::glBindVertexArray(id);
      lambda();
      GL46_Base::glBindVertexArray(0);
    }
};