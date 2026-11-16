#include "shared/pch.hh"

#include "shared/world/fluid_registry.hh"

#include "core/exception.hh"

#include "shared/mod_context.hh"

static std::vector<FluidDefinition> s_definitions;
static vx::hash_map<Identifier, fluid_id_type> s_names;
static vx::hash_map<fluid_id_type, Identifier> s_reverse_names;
static std::uint64_t s_checksum;

static void update_checksum(void)
{
    // TODO: go through each fluid definition and compute a checksum based on its contents
}

std::span<const FluidDefinition> fluid_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t fluid_registry::checksum(void)
{
    return s_checksum;
}

void fluid_registry::resolve_tints(void)
{
    for(auto& def : s_definitions) {
        if(def.tint_name.is_empty()) {
            def.tint = TINT_ID_NULL;
            continue;
        }

        def.tint = tint_registry::find(def.tint_name);

        if(def.tint == TINT_ID_NULL) {
            msg::log_warning("fluid {} references unknown tint {}", def.tint_name.full_string(), def.tint_name.full_string());
        }
    }
}

void fluid_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto fluids = ctx.take_fluids();
    auto names = ctx.take_fluid_names();

    fluid_id_type fluid_offset;

    if(fluids.empty()) {
        fluid_offset = static_cast<fluid_id_type>(s_definitions.size());
    }
    else {
        fluid_offset = static_cast<fluid_id_type>(s_definitions.size()) - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + fluid_offset;
        auto [it, inserted] = s_names.try_emplace(name, global_id);

        if(!inserted) {
            msg::log_warning("duplicate fluid name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(fluids.size()) {
        s_definitions.insert(s_definitions.end(), std::make_move_iterator(fluids.begin() + 1), std::make_move_iterator(fluids.end()));
    }

    update_checksum();
}

void fluid_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();
}

fluid_id_type fluid_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend())
        return FLUID_ID_NULL;
    return it->second;
}

std::optional<Identifier> fluid_registry::name_of(fluid_id_type id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const FluidDefinition* fluid_registry::find_definition(fluid_id_type id)
{
    if(id == FLUID_ID_NULL || id >= s_definitions.size())
        return nullptr;
    return &s_definitions[id];
}

const FluidDefinition* fluid_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}
