#ifndef C7905765_3FDE_4BA6_BF70_AEC7A901090D
#define C7905765_3FDE_4BA6_BF70_AEC7A901090D

#include "shared/mod_context.hh"

namespace mod_loader::detail
{
void add_library(library_opener opener);
} // namespace mod_loader::detail

namespace mod_loader
{
void init(void);
void shutdown(void);
} // namespace mod_loader

namespace mod_loader
{
template<auto Candidate>
void add_library(void);
} // namespace mod_loader

namespace mod_loader
{
std::span<const ModContext> all(void);
} // namespace mod_loader

namespace mod_loader
{
const ModContext* find(std::string_view name_space);
} // namespace mod_loader

template<auto Candidate>
void mod_loader::add_library(void)
{
    library_opener opener;
    opener.connect<Candidate>();
    detail::add_library(std::move(opener));
}

#endif /* C7905765_3FDE_4BA6_BF70_AEC7A901090D */
