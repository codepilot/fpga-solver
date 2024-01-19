#pragma once

#include "gl46_base.h"

template<constexpr_string label>
class GL_VertexArray : public GL_Label<GL_Label_Type::VERTEX_ARRAY, label> {
public:
    inline static GLuint create_id() {
        GLuint id{};
        GL46_Base::glCreateVertexArrays(1, &id);
        return id;
    }
    inline GL_VertexArray(): GL_Label<GL_Label_Type::VERTEX_ARRAY, label>(create_id()) {
    }
    inline void bind(auto lambda) const {
      GL46_Base::glBindVertexArray(this->id);
      lambda();
      GL46_Base::glBindVertexArray(0);
    }
    inline void attribBinding(GLuint attribindex, GLuint bindingindex) {
        GL46_Base::glVertexArrayAttribBinding(this->id, attribindex, bindingindex);

    }
    inline void attribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
        GL46_Base::glVertexArrayAttribFormat(this->id, attribindex, size, type, normalized, relativeoffset);

    }
    template<constexpr_string label>
    inline void attribBuffer(GLuint bindingindex, GL_Buffer<label>& buffer, GLintptr offset, GLsizei stride) {
        GL46_Base::glVertexArrayVertexBuffer(this->id, bindingindex, buffer.id, offset, stride);

    }
    inline void attribEnable(GLuint attribindex) {
        GL46_Base::glEnableVertexArrayAttrib(this->id, attribindex);

    }
    template<constexpr_string label>
    inline void elementBuffer(GL_Buffer<label> &buffer) {
        GL46_Base::glVertexArrayElementBuffer(this->id, buffer.id);

    }
};