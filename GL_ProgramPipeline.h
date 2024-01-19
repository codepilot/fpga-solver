#pragma once

#include "gl46_base.h"

#include "GL_Program.h"

template<constexpr_string label>
class GL_ProgramPipeline : public GL_Label<GL_Label_Type::PROGRAM_PIPELINE, label> {
public:
    inline GLuint static create_id() {
        GLuint id{};
        GL46_Base::glCreateProgramPipelines(1, &id);
        return id;
    }
    inline GL_ProgramPipeline() : GL_Label<GL_Label_Type::PROGRAM_PIPELINE, label>{ create_id() } {
    }
    inline void bind(auto lambda) const {
      GL46_Base::glBindProgramPipeline(this->id);
      lambda();
      GL46_Base::glBindProgramPipeline(0);
    }
    inline void validate() {
        GL46_Base::glValidateProgramPipeline(this->id);
    }
};