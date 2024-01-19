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
    inline static GL_Shader spirv_span(std::span<unsigned char> spirv) {
        GL_Shader shader;

        // Apply the vertex shader SPIR-V to the shader object.
        GL46_Base::glShaderBinary(1, &shader.id, GL_SHADER_BINARY_FORMAT_SPIR_V, spirv.data(), static_cast<GLsizei>(spirv.size()));

        GL46_Base::glSpecializeShader(shader.id, (const GLchar*)"main", 0, nullptr, nullptr);

        return shader;
    }
    inline static GL_Shader spirv_filename(GL_ShaderType shaderType, std::string spirv_filename) {
        MemoryMappedFile mmf{ spirv_filename };
        return spirv_span(shaderType, mmf.get_span<unsigned char>());
    }


};