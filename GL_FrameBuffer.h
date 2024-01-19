#pragma once

#include "gl46_base.h"

template<constexpr_string label>
class GL_FrameBuffer : public GL_Label<GL_Label_Type::FRAMEBUFFER, label> {
public:
    inline static GLuint create_id() {
        GLuint id{};
        GL46_Base::glCreateFramebuffers(1, &id);
        return id;
    }
    inline GL_FrameBuffer(): GL_Label<GL_Label_Type::FRAMEBUFFER, label>(create_id()) {
    }
};