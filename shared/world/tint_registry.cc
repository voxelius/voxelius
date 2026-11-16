#include "shared/pch.hh"

#include "shared/world/tint_registry.hh"

#include "shared/mod_context.hh"

static std::vector<TintDefinition> s_definitions;
static vx::hash_map<Identifier, tint_id_type> s_names;
static vx::hash_map<tint_id_type, Identifier> s_reverse_names;
static std::uint64_t s_checksum;

static void update_checksum(void)
{
    // TODO: go through each tint definition and compute a checksum based on its contents
}

std::span<const TintDefinition> tint_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t tint_registry::checksum(void)
{
    return s_checksum;
}

void tint_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto tints = ctx.take_tints();
    auto names = ctx.take_tint_names();

    tint_id_type tint_offset;

    if(tints.empty()) {
        tint_offset = static_cast<tint_id_type>(s_definitions.size());
    }
    else {
        tint_offset = static_cast<tint_id_type>(s_definitions.size()) - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + tint_offset;
        auto [it, inserted] = s_names.try_emplace(name, global_id);

        if(!inserted) {
            msg::log_warning("duplicate tint name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(tints.size()) {
        s_definitions.insert(s_definitions.end(), std::make_move_iterator(tints.begin() + 1), std::make_move_iterator(tints.end()));
    }

    update_checksum();
}

void tint_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();
}

tint_id_type tint_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend())
        return TINT_ID_NULL;
    return it->second;
}

std::optional<Identifier> tint_registry::name_of(tint_id_type id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const TintDefinition* tint_registry::find_definition(tint_id_type id)
{
    if(id == TINT_ID_NULL || id >= s_definitions.size())
        return nullptr;
    return &s_definitions[id];
}

const TintDefinition* tint_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}
