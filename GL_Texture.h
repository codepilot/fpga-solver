#pragma once

template<GLenum target>
class GL_Texture {
public:
    GLuint id;
    ~GL_Texture() {
        OutputDebugStringW(L"~GL_Texture()\r\n");
        GL46_Base::glDeleteTextures(1, &id); id = 0;
    }
    static std::vector<GL_Texture> create(size_t count = 0) {
        std::vector<GL_Texture> textures(count, 0);
        if (count > 0) {
            GL46_Base::glCreateTextures(target, count, textures.data());
            OutputDebugStringW(L"glCreateTextures\r\n");
        }
        return textures;
    }
    static void bindImages(std::span<GL_Texture> textures) {
        GL46_Base::glBindImageTextures(0, textures.size(), textures.data());
    }
};

template<GLenum target>
class GL_Textures {
    std::vector<GLuint> vec;
public:
    GLsizei sizei() {
        return SafeInt<GLsizei>(vec.size());
    }
    GL_Textures(const GL_Textures&) = delete;
    GL_Textures& operator=(const GL_Textures&) = delete;
    GL_Textures(GL_Textures&& src) {
        vec = std::move(src.vec);
    }
    GL_Textures& operator=(GL_Textures&& src) {
        vec = std::move(src.vec);
        return *this;
    }
    GL_Textures(GLsizei n = 0) {
        vec.resize(n);
        if (sizei() > 0) {
            GL46_Base::glCreateTextures(target, sizei(), vec.data());
            OutputDebugStringW(L"glCreateTextures\r\n");
        }
    }
    ~GL_Textures() {
        if (sizei() > 0 && gl) {
            GL46_Base::glDeleteTextures(sizei(), vec.data());
            OutputDebugStringW(L"glDeleteTextures\r\n");
        }
    }
    void bindImages() {
        GL46_Base::glBindImageTextures(0, sizei(), vec.data());
    }
    GLuint operator [](GLsizei i) {
        return vec[i];
    }
};