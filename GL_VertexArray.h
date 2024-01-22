#pragma once

#include "gl46_base.h"

template<constexpr_string label>
class GL_VertexArray : public GL_Label<GL_Label_Type::VERTEX_ARRAY, label> {
public:
    class VA_Attrib_Binding {
    public:
        GL_VertexArray& va;
        GLuint attribindex;
        GLuint bindingindex;
        inline void bindingDivisor(GLuint divisor) {
            GL46_Base::glVertexArrayBindingDivisor(va.id, bindingindex, divisor);

        }
        inline void attribFormat(GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
            GL46_Base::glVertexArrayAttribFormat(va.id, attribindex, size, type, normalized, relativeoffset);

        }
        template<constexpr_string label>
        inline void attribBuffer(GL_Buffer<label>& buffer, GLintptr offset, GLsizei stride) {
            GL46_Base::glVertexArrayVertexBuffer(va.id, bindingindex, buffer.id, offset, stride);

        }
        inline void attribEnable() {
            GL46_Base::glEnableVertexArrayAttrib(va.id, attribindex);

        }
    };
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
    inline VA_Attrib_Binding attribBinding(GLuint attribindex, GLuint bindingindex) {
        GL46_Base::glVertexArrayAttribBinding(this->id, attribindex, bindingindex);
        return VA_Attrib_Binding{
            .va{*this},
            .attribindex{attribindex},
            .bindingindex{bindingindex},
        };
    }
    template<constexpr_string label>
    inline void elementBuffer(GL_Buffer<label> &buffer) {
        GL46_Base::glVertexArrayElementBuffer(this->id, buffer.id);

    }
};