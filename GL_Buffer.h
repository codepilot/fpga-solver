#pragma once

class GL_Buffer {
public:
    GLuint id;
    ~GL_Buffer() {
        OutputDebugStringW(L"~GL_Buffer()\r\n");
        GL46_Base::glDeleteBuffers(1, &id); id = 0;
    }
    static std::vector<GL_Buffer> create(size_t count = 0) {
        std::vector<GL_Buffer> textures(count);
        if (count > 0) {
            GL46_Base::glCreateBuffers(static_cast<GLsizei>(count), reinterpret_cast<GLuint *>(textures.data()));
            OutputDebugStringW(L"glCreateBuffers\r\n");
        }
        return textures;
    }
};

static_assert(sizeof(GL_Buffer) == sizeof(GLuint));