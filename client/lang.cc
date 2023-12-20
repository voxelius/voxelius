// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include <client/event/lang_set.hh>
#include <client/globals.hh>
#include <client/lang.hh>
#include <entt/signal/dispatcher.hpp>
#include <parson.h>
#include <shared/config/config.hh>
#include <shared/config/string.hh>
#include <shared/vfs.hh>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

static lang_vector_t manifest = {};
static lang_iterator_t current_lang = {};
static std::unordered_map<std::string, std::string> lang_kv = {};
static std::unordered_map<std::string, lang_iterator_t> ietf_lookup = {};
static config::String config_language = config::String{"en_US"};

static inline void send_lang_set_event(lang_iterator_t language)
{
    LangSetEvent event = {};
    event.it = language;
    globals::dispatcher.trigger(event);
}

void lang::init()
{
    config::add("language", config_language);

    // Available languages are kept in a
    // special manifest file which consists
    // of keys in an IETF-ish format and values
    // as the according language's endonym
    const vfs::path_t path = "/lang/manifest.json";

    std::string source = {};

    if(!vfs::read_string(path, source)) {
        spdlog::error("lang: {}: {}", path.string(), vfs::get_error());
        std::terminate();
    }

    JSON_Value *jsonv = json_parse_string(source.c_str());
    const JSON_Object *json = json_value_get_object(jsonv);
    const size_t count = json_object_get_count(json);

    if(!jsonv || !json || !count) {
        spdlog::error("lang: {}: parse error", path.string());
        std::terminate();
    }

    manifest.clear();
    ietf_lookup.clear();

    for(size_t i = 0; i < count; ++i) {
        const char *ietf = json_object_get_name(json, i);
        const JSON_Value *val = json_object_get_value_at(json, i);
        const char *endonym = json_value_get_string(val);

        if(ietf && endonym) {
            LangInfo info = {};
            info.ietf = std::string{ietf};
            info.endonym = std::string{endonym};
            info.display = fmt::format("{} ({})", endonym, ietf);
            manifest.push_back(info);
        }
    }

    for(lang_iterator_t it = manifest.cbegin(); it != manifest.cend(); ++it) {
        ietf_lookup.emplace(it->ietf, it);
    }

    json_value_free(jsonv);

    // This is temporary! This value will
    // be overriden in init_late after the
    // config system updates config_language
    current_lang = manifest.cend();
}

void lang::init_late()
{
    if(const auto it = ietf_lookup.find(config_language.value); it != ietf_lookup.cend()) {
        lang::set(it->second);
        return;
    }

    if(const auto it = ietf_lookup.find("en_US"); it != ietf_lookup.cend()) {
        lang::set(it->second);
        return;
    }

    spdlog::error("lang: we're doomed!");
    spdlog::error("lang: en_US doesn't exist!");
    std::terminate();
}

void lang::set(lang_iterator_t language)
{
    if(language != manifest.cend()) {
        const vfs::path_t path = fmt::format("/lang/lang.{}.json", language->ietf);

        std::string source = {};

        if(!vfs::read_string(path, source)) {
            spdlog::warn("lang: {}: {}", path.string(), vfs::get_error());
            send_lang_set_event(language);
            return;
        }

        JSON_Value *jsonv = json_parse_string(source.c_str());
        const JSON_Object *json = json_value_get_object(jsonv);
        const size_t count = json_object_get_count(json);

        if(!jsonv || !json || !count) {
            spdlog::warn("lang: {}: parse error", path.string());
            send_lang_set_event(language);
            return;
        }

        lang_kv.clear();

        for(size_t i = 0; i < count; ++i) {
            const char *key = json_object_get_name(json, i);
            const JSON_Value *val = json_object_get_value_at(json, i);
            const char *val_str = json_value_get_string(val);

            if(key && val_str) {
                lang_kv.emplace(key, val_str);
                continue;
            }
        }

        json_value_free(jsonv);

        current_lang = language;
        config_language = language->ietf;
    }

    send_lang_set_event(language);
}

const std::string &lang::find(const std::string &key)
{
    if(const auto it = lang_kv.find(key); it != lang_kv.cend())
        return it->second;
    return key;
}

lang_iterator_t lang::lookup(const std::string &ietf)
{
    if(const auto it = ietf_lookup.find(ietf); it != ietf_lookup.cend())
        return it->second;
    return manifest.cend();
}

lang_iterator_t lang::current()
{
    return current_lang;
}

lang_iterator_t lang::cbegin()
{
    return manifest.cbegin();
}

lang_iterator_t lang::cend()
{
    return manifest.cend();
}
