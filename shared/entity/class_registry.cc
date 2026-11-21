#include "shared/pch.hh"

#include "shared/entity/class_registry.hh"

#include "shared/mod_context.hh"

static std::vector<ClassDefinition> s_definitions;
static vx::hash_map<Identifier, class_id_type> s_names;
static vx::hash_map<class_id_type, Identifier> s_reverse_names;
static std::uint64_t s_checksum;

static void update_checksum(void)
{
    // TODO: go through each class definition and compute a checksum based on its contents
}

std::span<const ClassDefinition> class_registry::all_definitions(void)
{
    return s_definitions;
}

std::uint64_t class_registry::checksum(void)
{
    return s_checksum;
}

void class_registry::commit(ModContext& ctx)
{
    if(s_definitions.empty()) {
        s_definitions.emplace_back();
    }

    auto classes = ctx.take_classes();
    auto names = ctx.take_class_names();

    auto class_count = static_cast<class_id_type>(s_definitions.size());

    class_id_type class_offset;

    if(classes.empty()) {
        class_offset = class_count;
    }
    else {
        class_offset = class_count - 1;
    }

    for(const auto& entry : names) {
        auto& name = entry.first;
        auto global_id = entry.second + class_offset;
        auto [it, inserted] = s_names.try_emplace(name, global_id);

        if(!inserted) {
            msg::log_warning("duplicate class name: {}", name.full_string());
            continue;
        }

        s_reverse_names.try_emplace(global_id, name);
    }

    if(classes.size()) {
        auto source_begin = std::next(classes.begin());
        auto source_end = classes.end();
        auto first = std::make_move_iterator(source_begin);
        auto last = std::make_move_iterator(source_end);
        auto destination = s_definitions.end();

        s_definitions.insert(destination, first, last);
    }
}

void class_registry::purge(void)
{
    s_definitions.clear();
    s_names.clear();
    s_reverse_names.clear();
    s_checksum = 0;
}

class_id_type class_registry::find(const Identifier& id)
{
    auto it = s_names.find(id);

    if(it == s_names.cend())
        return CLASS_ID_NULL;
    return it->second;
}

std::optional<Identifier> class_registry::name_of(class_id_type id)
{
    auto it = s_reverse_names.find(id);

    if(it == s_reverse_names.cend()) {
        return std::nullopt;
    }

    return it->second;
}

const ClassDefinition* class_registry::find_definition(class_id_type id)
{
    if(id == CLASS_ID_NULL || id >= s_definitions.size())
        return nullptr;
    return &s_definitions[id];
}

const ClassDefinition* class_registry::find_definition(const Identifier& id)
{
    return find_definition(find(id));
}

void class_registry::for_each(const ClassDefinition* def, const std::function<void(component_id_type)>& callback)
{
    assert(callback);
    assert(def);

    for(auto& entry : def->entries) {
        callback(entry.id);
    }
}

void class_registry::for_each(class_id_type class_id, const std::function<void(component_id_type)>& callback)
{
    assert(callback);

    if(auto def = find_definition(class_id)) {
        for_each(def, callback);
    }
}

void class_registry::for_each(const Identifier& id, const std::function<void(component_id_type)>& callback)
{
    assert(callback);

    if(auto def = find_definition(id)) {
        for_each(def, callback);
    }
}
