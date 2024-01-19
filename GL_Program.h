#pragma once

#include "gl46_base.h"

#include "GL_Shader.h"

class GL_Program {
public:
    GLuint id;
    ~GL_Program() {
        OutputDebugStringW(L"~glDeleteProgram()\r\n");
        GL46_Base::glDeleteProgram(id); id = 0;
    }
    static GL_Program create() {
        GLuint id{ GL46_Base::glCreateProgram() };
        OutputDebugStringW(L"glCreatePrograms\r\n");
        return GL_Program{.id{id}};
    }
	inline static GL_Program spirv_span(GL_ShaderType shaderType, std::span<unsigned char> spirv) {
		// Read our shaders into the appropriate buffers

		// Create an empty vertex shader handle
		GL_Shader shader{ GL_Shader::spirv_span(shaderType, spirv) };

		GL_Program program{ create() };

		// Attach our shaders to our program
		GL46_Base::glProgramParameteri(program.id, GL_PROGRAM_SEPARABLE, GL_TRUE);
		GL46_Base::glAttachShader(program.id, shader.id);

		// Link our program
		GL46_Base::glLinkProgram(program.id);

		// Always detach shaders after a successful link.
		GL46_Base::glDetachShader(program.id, shader.id);
		return program;
	}

#if 0
	inline static GL_Program glsl(GL_ShaderType shaderType, std::string src) {
		std::array<const char*, 1> strings{ src.c_str() };
		const auto prog = GL46_Base::glCreateShaderProgramv(
			static_cast<GLenum>(shaderType),
			SafeInt<GLsizei>(strings.size()),
			strings.data());
		GLint linkStatus{};
		GLint validateStatus{};
		// glValidateProgram(prog);
		GL46_Base::glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
		GL46_Base::glGetProgramiv(prog, GL_VALIDATE_STATUS, &validateStatus);
		GLint infoLogLength{ 0 };
		GL46_Base::glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength) {
			std::vector<GLchar> infoLog; infoLog.resize(infoLogLength);
			GLsizei infoLogSize{ 0 };
			GL46_Base::glGetProgramInfoLog(prog, SafeInt<GLsizei>(infoLog.size()), &infoLogSize, infoLog.data());
			std::string infoLogStr{ infoLog.begin(), infoLog.end() };
			infoLogStr += "\r\n";
			OutputDebugStringA(infoLogStr.c_str());
			if (!linkStatus || !validateStatus) {
				DebugBreak();
			}
		}
		return prog;
	}
#endif
};