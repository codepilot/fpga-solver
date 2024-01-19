#pragma once

#include "gl46_base.h"

class GL_ProgramPipeline {
public:
    GLuint id;
    ~GL_ProgramPipeline() {
        OutputDebugStringW(L"~glDeleteProgramPipelines()\r\n");
        GL46_Base::glDeleteProgramPipelines(1, &id); id = 0;
    }
    static GL_ProgramPipeline create() {
        GLuint id{};
        GL46_Base::glCreateProgramPipelines(1, &id);
        OutputDebugStringW(L"glCreateProgramPipelines\r\n");
        return GL_ProgramPipeline{.id{id}};
    }
    void bind(auto lambda) const {
      GL46_Base::glBindProgramPipeline(id);
      lambda();
      GL46_Base::glBindProgramPipeline(0);
    }
};