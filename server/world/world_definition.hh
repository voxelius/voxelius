#ifndef C4A8E2F1_7D3B_4A96_B5C8_1E9F3D7A2B64
#define C4A8E2F1_7D3B_4A96_B5C8_1E9F3D7A2B64

#include "shared/world/world_definition.hh"

struct GeneratorDefinition;

struct ServerWorldDefinition final : WorldDefinition {
    const GeneratorDefinition* generator { nullptr };
    std::shared_ptr<void> generator_state;

    bool is_default { false };
};

#endif /* C4A8E2F1_7D3B_4A96_B5C8_1E9F3D7A2B64 */
