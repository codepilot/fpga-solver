#pragma once

template<GLenum target, constexpr_string label>
class GL_Texture : public GL_Label<GL_Label_Type::TEXTURE, label> {
public:
    inline static GLuint create_id() {
        GLuint id{};
        GL46_Base::glCreateTextures(target, 1, &id);
        return id;
    }
    inline GL_Texture(): GL_Label<GL_Label_Type::TEXTURE, label>(create_id()) {
    }
};
