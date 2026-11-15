#include "client/pch.hh"

#include "client/language.hh"

#include "core/cvar/host.hh"
#include "core/cvar/string.hh"
#include "core/exception.hh"
#include "core/identifier.hh"
#include "core/utils/physfs.hh"

#include "shared/mod_context.hh"
#include "shared/mod_loader.hh"

#include "client/constant.hh"
#include "client/globals.hh"
#include "client/gui/container.hh"
#include "client/gui/language_selector.hh"
#include "client/settings.hh"

constexpr static std::string_view DEFAULT_LANGUAGE = "english";

static language_manifest_type s_manifest;
static language_iterator_type s_current_language;
static vx::hash_map<std::string, std::string> s_translations;
static vx::hash_map<std::string, language_iterator_type> s_code_map;
static cvar::String s_lang_current { std::string(DEFAULT_LANGUAGE) };
static gui::LanguageSelector s_language_selector;

static void read_mod_translations(std::string_view name_space)
{
    auto path = Identifier::from_parts(name_space, std::format("lang.{}", s_current_language->code())).as_file_path("lang", ".json");
    std::string source;

    if(!utils::read_file(path, source)) {
        msg::log_debug("{}: read failed", path);
        return;
    }

    auto jsonv = json_parse_string(source.c_str());
    auto json = json_value_get_object(jsonv);

    if(jsonv == nullptr || json == nullptr) {
        msg::log_warning("{}: parse error", path);
        return;
    }

    auto count = json_object_get_count(json);

    for(std::size_t i = 0; i < count; ++i) {
        auto key = json_object_get_name(json, i);
        auto value = json_object_get_value_at(json, i);
        auto translation = json_value_get_string(value);

        if(key && translation) {
            s_translations.insert_or_assign(std::string(key), std::string(translation));
        }
    }

    json_value_free(jsonv);
}

LanguageInfo::LanguageInfo(std::string endonym, std::string display, std::string code)
    : m_endonym(std::move(endonym)), m_display(std::move(display)), m_code(std::move(code))
{
    // empty
}

LanguageUpdateEvent::LanguageUpdateEvent(language_iterator_type lang) : m_language(lang)
{
    // empty
}

static void update_language(void)
{
    auto lang = s_code_map.find(s_lang_current.value());

    if(lang == s_code_map.cend()) {
        msg::log_warning("{}: unknown language, falling back to {}", s_lang_current.value(), DEFAULT_LANGUAGE);

        lang = s_code_map.find(DEFAULT_LANGUAGE);

        vx::throw_if_fmt(lang == s_code_map.cend(), "we are doomed: {} doesn't exist!!!", DEFAULT_LANGUAGE);

        s_lang_current.set_value(std::string(lang->second->code()));
        return;
    }

    if(lang->second == s_current_language) {
        return;
    }

    language::set(lang->second);
}

void language::init(void)
{
    auto manifest_path = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "manifest").as_file_path("lang", ".json");

    auto& lang_binding = cvar::bind_var(s_lang_current, "lang");
    lang_binding.set_help("Language");
    lang_binding.set_flags(FCVAR_ARCHIVE);
    lang_binding.changed().connect<&update_language>();

    s_language_selector.set_key("lang");
    settings::general.add_child(s_language_selector, 0);

    std::string source;
    auto read_ok = utils::read_file(manifest_path, source);
    vx::throw_if_not_fmt(read_ok, "{}: read failed", manifest_path);

    auto jsonv = json_parse_string(source.c_str());
    auto json = json_value_get_object(jsonv);
    vx::throw_if_not_fmt(jsonv, "{}: parse error", manifest_path);
    vx::throw_if_not_fmt(json, "{}: object expected", manifest_path);

    auto count = json_object_get_count(json);
    vx::throw_if_fmt(count == 0, "{}: no languages found", manifest_path);

    for(std::size_t i = 0; i < count; ++i) {
        auto code = json_object_get_name(json, i);
        auto value = json_object_get_value_at(json, i);
        auto endonym = json_value_get_string(value);

        if(code && endonym) {
            auto display = std::format("{} ({})", endonym, code);
            s_manifest.emplace_back(endonym, display, code);
        }
    }

    for(auto it = s_manifest.cbegin(); it != s_manifest.cend(); it = std::next(it)) {
        s_code_map.insert_or_assign(std::string(it->code()), language_iterator_type(it));
    }

    json_value_free(jsonv);

    // This is temporary! This value will
    // be overriden in init_late after the
    // cvar system updates the current language
    s_current_language = s_manifest.cend();
}

void language::init_late(void)
{
    update_language();
}

void language::set(language_iterator_type lang)
{
    if(lang == s_current_language) {
        globals::dispatcher.trigger(LanguageUpdateEvent(lang));
        return;
    }

    s_translations.clear();
    s_current_language = lang;

    if(s_lang_current.value() != lang->code()) {
        s_lang_current.set_value(std::string(lang->code()));
    }

    for(auto& ctx : mod_loader::all()) {
        read_mod_translations(ctx.name_space());
    }

    globals::dispatcher.trigger(LanguageUpdateEvent(lang));
}

language_iterator_type language::current(void)
{
    return s_current_language;
}

language_iterator_type language::find(std::string_view code)
{
    const auto it = s_code_map.find(code);

    if(it == s_code_map.cend()) {
        return s_manifest.cend();
    }
    else {
        return it->second;
    }
}

language_iterator_type language::const_begin(void)
{
    return s_manifest.cbegin();
}

language_iterator_type language::const_end(void)
{
    return s_manifest.cend();
}

std::string_view language::resolve(std::string_view key)
{
    const auto it = s_translations.find(key);

    if(it == s_translations.cend()) {
        return key;
    }
    else {
        return it->second;
    }
}

std::string language::resolve_gui(std::string_view key)
{
    // We need window tags to retain their hierarchy when a language
    // dynamically changes; ImGui allows to provide hidden unique identifiers
    // to GUI primitives that have their name change dynamically, so we're using this
    return std::format("{}###{}", language::resolve(key), key);
}
