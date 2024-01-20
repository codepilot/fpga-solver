#pragma once

#include "gl46_base.h"
#include "MemoryMappedFile.h"

enum class GL_ShaderType : GLenum {
    compute = GL_COMPUTE_SHADER,
    vertex = GL_VERTEX_SHADER,
    tess_control = GL_TESS_CONTROL_SHADER,
    tess_evaluation = GL_TESS_EVALUATION_SHADER,
    geometry = GL_GEOMETRY_SHADER,
    fragment = GL_FRAGMENT_SHADER,
};

template<constexpr_string label, GL_ShaderType shaderType>
class GL_Shader : public GL_Label<GL_Label_Type::SHADER, label> {
public:
    inline static GLuint create_id() {
        GLuint id{ GL46_Base::glCreateShader(static_cast<GLenum>(shaderType)) };
        return id;
    }
    inline GL_Shader() : GL_Label<GL_Label_Type::SHADER, label>{ create_id() } {
    }
    inline GL_Shader(GLuint id) : GL_Label<GL_Label_Type::SHADER, label>{ id } {
    }
    inline static GL_Shader spirv_span(std::span<unsigned char> spirv) {
        GLuint id{ GL46_Base::glCreateShader(static_cast<GLenum>(shaderType)) };

        // Apply the vertex shader SPIR-V to the shader object.
        GL46_Base::glShaderBinary(1, &id, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), static_cast<GLsizei>(spirv.size()));
        GL46_Base::glSpecializeShader(id, (const GLchar*)"main", 0, nullptr, nullptr);

        GLint compileStatus{};
        // glValidateProgram(prog);
        GL46_Base::glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
        GLint infoLogLength{ 0 };
        GL46_Base::glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength) {
            std::vector<GLchar> infoLog; infoLog.resize(infoLogLength);
            GLsizei infoLogSize{ 0 };
            GL46_Base::glGetShaderInfoLog(id, SafeInt<GLsizei>(infoLog.size()), &infoLogSize, infoLog.data());
            std::string infoLogStr{ infoLog.begin(), infoLog.end() };
            infoLogStr += "\r\n";
            OutputDebugStringA(infoLogStr.c_str());
            if (!compileStatus) {
                DebugBreak();
            }
        }

        return id;
    }
    inline static GL_Shader spirv_filename(GL_ShaderType shaderType, std::string spirv_filename) {
        MemoryMappedFile mmf{ spirv_filename };
        return spirv_span(shaderType, mmf.get_span<unsigned char>());
    }


};