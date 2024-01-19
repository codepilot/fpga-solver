#pragma once

template<GLenum target, constexpr_string label>
class GL_Texture : public GL_Label<GL_Label_Type::TEXTURE, label> {
public:
    inline static GLuint create_id() {
        GLuint id{};
        GL46_Base::glCreateTextures(target, 1, &id);
        return id;
    }
    inline GL_Texture(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height): GL_Label<GL_Label_Type::TEXTURE, label>(create_id()) {
        GL46_Base::glTextureStorage2D(this->id, levels, internalformat, width, height);
    }
    inline void clear(GLint level, GLenum format, GLenum type, const void* data) {
        GL46_Base::glClearTexImage(this->id, level, format, type, data);
    }
};
