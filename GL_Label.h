#pragma once

#include "gl46_base.h"
#include "Trivial_Span.h"
#include "constexpr_string.h"

enum class GL_Label_Type : GLenum {
	BUFFER = GL_BUFFER,
	SHADER = GL_SHADER,
	PROGRAM = GL_PROGRAM,
	VERTEX_ARRAY = GL_VERTEX_ARRAY,
	QUERY = GL_QUERY,
	PROGRAM_PIPELINE = GL_PROGRAM_PIPELINE,
	TRANSFORM_FEEDBACK = GL_TRANSFORM_FEEDBACK,
	SAMPLER = GL_SAMPLER,
	TEXTURE = GL_TEXTURE,
	RENDERBUFFER = GL_RENDERBUFFER,
	FRAMEBUFFER = GL_FRAMEBUFFER,
};

template<GL_Label_Type identifier, constexpr_string label>
class GL_Label {
public:
	const GLuint id;
	inline GL_Label(GLuint id) : id{ id } {
		GL46_Base::glObjectLabel(static_cast<GLenum>(identifier), id, label.size(), label.data());
		switch (identifier) {
		case GL_Label_Type::BUFFER: {
			OutputDebugStringA(std::format("BUFFER {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::SHADER: {
			OutputDebugStringA(std::format("SHADER {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::PROGRAM: {
			OutputDebugStringA(std::format("PROGRAM {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::VERTEX_ARRAY: {
			OutputDebugStringA(std::format("VERTEX_ARRAY {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::QUERY: {
			OutputDebugStringA(std::format("QUERY {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::PROGRAM_PIPELINE: {
			OutputDebugStringA(std::format("PROGRAM_PIPELINE {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::TRANSFORM_FEEDBACK: {
			OutputDebugStringA(std::format("TRANSFORM_FEEDBACK {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::SAMPLER: {
			OutputDebugStringA(std::format("SAMPLER {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::TEXTURE: {
			OutputDebugStringA(std::format("TEXTURE {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::RENDERBUFFER: {
			OutputDebugStringA(std::format("RENDERBUFFER {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		case GL_Label_Type::FRAMEBUFFER: {
			OutputDebugStringA(std::format("FRAMEBUFFER {}:{}\r\n", label.chars, id).c_str());
			return;
		}
		default: {
			std::unreachable();
		}
		}
	}
	inline ~GL_Label() {
		switch (identifier) {
		case GL_Label_Type::BUFFER: {
			OutputDebugStringA(std::format("~BUFFER {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteBuffers(1, &id);
			return;
		}
		case GL_Label_Type::SHADER: {
			OutputDebugStringA(std::format("~SHADER {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteShader(id);
			return;
		}
		case GL_Label_Type::PROGRAM: {
			OutputDebugStringA(std::format("~PROGRAM {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteProgram(id);
			return;
		}
		case GL_Label_Type::VERTEX_ARRAY: {
			OutputDebugStringA(std::format("~VERTEX_ARRAY {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteVertexArrays(1, &id);
			return;
		}
		case GL_Label_Type::QUERY: {
			OutputDebugStringA(std::format("~QUERY {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteQueries(1, &id);
			return;
		}
		case GL_Label_Type::PROGRAM_PIPELINE: {
			OutputDebugStringA(std::format("~PROGRAM_PIPELINE {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteProgramPipelines(1, &id);
			return;
		}
		case GL_Label_Type::TRANSFORM_FEEDBACK: {
			OutputDebugStringA(std::format("~TRANSFORM_FEEDBACK {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteTransformFeedbacks(1, &id);
			return;
		}
		case GL_Label_Type::SAMPLER: {
			OutputDebugStringA(std::format("~SAMPLER {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteSamplers(1, &id);
			return;
		}
		case GL_Label_Type::TEXTURE: {
			OutputDebugStringA(std::format("~TEXTURE {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteTextures(1, &id);
			return;
		}
		case GL_Label_Type::RENDERBUFFER: {
			OutputDebugStringA(std::format("~RENDERBUFFER {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteRenderbuffers(1, &id);
			return;
		}
		case GL_Label_Type::FRAMEBUFFER: {
			OutputDebugStringA(std::format("~FRAMEBUFFER {}:{}\r\n", label.chars, id).c_str());
			GL46_Base::glDeleteFramebuffers(1, &id);
			return;
		}
		default: {
			std::unreachable();
		}
		}
	}
};