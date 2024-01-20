#pragma once

#include "gl46_base.h"

#include "GL_Shader.h"

template<constexpr_string label, GL_ShaderType shaderType>
class GL_Program: public GL_Label<GL_Label_Type::PROGRAM, label> {
public:
	inline GLuint static create_id(std::span<unsigned char> glsl) {
		GLchar *ptr{ reinterpret_cast<GLchar *>(glsl.data()) };
		GLuint id{ GL46_Base::glCreateShaderProgramv(static_cast<GLenum>(shaderType), 1, &ptr) };
		GL46_Base::glLinkProgram(id);
		return id;
	}
	inline GLuint static create_id() {
		GLuint id{ GL46_Base::glCreateProgram() };
		return id;
	}
	inline GL_Program(std::span<unsigned char> glsl) : GL_Label<GL_Label_Type::PROGRAM, label>{ create_id(glsl) } {
    }
	inline GL_Program() : GL_Label<GL_Label_Type::PROGRAM, label>{ create_id() } {
	}
	inline GL_Program(GLuint id) : GL_Label<GL_Label_Type::PROGRAM, label>{ id } {
	}
	inline void uniform2f(GLint location, GLfloat v0, GLfloat v1) {
		GL46_Base::glProgramUniform2f(this->id, location, v0, v1);
	}
	inline void uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
		GL46_Base::glProgramUniform4f(this->id, location, v0, v1, v2, v3);
	}
	inline static GL_Program spirv_span(std::span<unsigned char> spirv) {
		// Read our shaders into the appropriate buffers

		// Create an empty vertex shader handle
		auto shader{ GL_Shader<label, shaderType>::spirv_span(spirv) };

		GLuint prog{ GL46_Base::glCreateProgram() };

		// Attach our shaders to our program
		GL46_Base::glProgramParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);
		GL46_Base::glAttachShader(prog, shader.id);

		GLint linkStatus{};
		GLint validateStatus{};
		GLint infoLogLength{ 0 };

		GL46_Base::glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
		GL46_Base::glGetProgramiv(prog, GL_VALIDATE_STATUS, &validateStatus);
		GL46_Base::glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength);

		// Link our program
		GL46_Base::glLinkProgram(prog);

		GL46_Base::glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
		GL46_Base::glGetProgramiv(prog, GL_VALIDATE_STATUS, &validateStatus);
		GL46_Base::glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength);

		GL46_Base::glValidateProgram(prog);

		// glValidateProgram(prog);
		GL46_Base::glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
		GL46_Base::glGetProgramiv(prog, GL_VALIDATE_STATUS, &validateStatus);
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


		// Always detach shaders after a successful link.
		GL46_Base::glDetachShader(prog, shader.id);
		return prog;
	}

	inline static GL_Program glsl(std::string src) {
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
};