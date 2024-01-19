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
};