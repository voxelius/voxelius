// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#ifndef CLIENT_GLXX_PROGRAM_PIPELINE_HH
#define CLIENT_GLXX_PROGRAM_PIPELINE_HH
#include <client/glxx/program.hh>

namespace glxx
{
class ProgramPipeline : public Object<ProgramPipeline> {
public:
    ProgramPipeline() = default;
    ProgramPipeline(ProgramPipeline &&rhs);
    ProgramPipeline &operator=(ProgramPipeline &&rhs);
    void create();
    void destroy();
    void stage(const Program &program);
    void bind() const;
    static void unbind();
};
} // namespace glxx

inline glxx::ProgramPipeline::ProgramPipeline(glxx::ProgramPipeline &&rhs)
{
    handle = rhs.handle;
    rhs.handle = 0;
}

inline glxx::ProgramPipeline &glxx::ProgramPipeline::operator=(glxx::ProgramPipeline &&rhs)
{
    glxx::ProgramPipeline copy(std::move(rhs));
    std::swap(handle, copy.handle);
    return *this;
}

inline void glxx::ProgramPipeline::create()
{
    destroy();
    glCreateProgramPipelines(1, &handle);
}

inline void glxx::ProgramPipeline::destroy()
{
    if(handle) {
        glDeleteProgramPipelines(1, &handle);
        handle = 0;
    }
}

inline void glxx::ProgramPipeline::stage(const glxx::Program &program)
{
    glUseProgramStages(handle, program.get_stage_bits(), program.get());
}

inline void glxx::ProgramPipeline::bind() const
{
    glBindProgramPipeline(handle);
}

inline void glxx::ProgramPipeline::unbind()
{
    glBindProgramPipeline(0);
}

#endif /* CLIENT_GLXX_PROGRAM_PIPELINE_HH */
