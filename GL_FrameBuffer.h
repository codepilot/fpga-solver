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
    inline void blitDefault(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
        GL46_Base::glBlitNamedFramebuffer(this->id, 0, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    }
    template<GLenum target, constexpr_string label>
    inline void texture(GLenum attachment, GL_Texture<target, label> &texture, GLint level) {
        GL46_Base::glNamedFramebufferTexture(this->id, GL_COLOR_ATTACHMENT0, texture.id, 0);
    }

};