#pragma once

class GL_Buffer {
public:
    GLuint id;
    ~GL_Buffer() {
        OutputDebugStringW(L"~GL_Buffer()\r\n");
        GL46_Base::glDeleteBuffers(1, &id); id = 0;
    }
    static GL_Buffer create(GLsizeiptr size, const void *data = nullptr, GLbitfield flags = 0) {
        GLuint id{};
        GL46_Base::glCreateBuffers(1, &id);
        GL46_Base::glNamedBufferStorage(id, size, data, flags);
        OutputDebugStringW(L"glCreateBuffers\r\n");
        return GL_Buffer{.id{id}};
    }
    static std::vector<GL_Buffer> create_vector(size_t count) {
        std::vector<GL_Buffer> ids(count, GL_Buffer{});
        if (count > 0) {
            GL46_Base::glCreateBuffers(static_cast<GLsizei>(count), reinterpret_cast<GLuint *>(ids.data()));
            OutputDebugStringW(L"glCreateBuffers\r\n");
        }
        return ids;
    }
};

static_assert(sizeof(GL_Buffer) == sizeof(GLuint));