#pragma once

template<constexpr_string label>
class GL_Buffer : public GL_Label<GL_Label_Type::BUFFER, label> {
public:
    inline static GLuint create_id(GLsizeiptr size, const void* data, GLbitfield flags) {
        GLuint id{};
        GL46_Base::glCreateBuffers(1, &id);
        GL46_Base::glNamedBufferStorage(id, size, data, flags);
        return id;
    }
    inline GL_Buffer(GLsizeiptr size, const void *data = nullptr, GLbitfield flags = 0): GL_Label<GL_Label_Type::BUFFER, label>(create_id(size, data, flags )) {
    }
    inline void bind(GLenum target, auto lambda) const {
        GL46_Base::glBindBuffer(target, this->id);
        lambda();
        GL46_Base::glBindBuffer(target, 0);
    }
};
