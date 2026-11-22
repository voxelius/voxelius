#ifndef E92E5ABE_D4FD_462C_89D4_BB75774051B7
#define E92E5ABE_D4FD_462C_89D4_BB75774051B7

class LanguageInfo final {
public:
    explicit LanguageInfo(std::string endonym, std::string display, std::string code);
    constexpr std::string_view endonym(void) const;
    constexpr std::string_view display(void) const;
    constexpr std::string_view code(void) const;
    constexpr const char* display_cstr(void) const;

private:
    std::string m_endonym;
    std::string m_display;
    std::string m_code;
};

using language_manifest_type = std::vector<LanguageInfo>;
using language_iterator_type = language_manifest_type::const_iterator;

class LanguageUpdateEvent final {
public:
    explicit LanguageUpdateEvent(language_iterator_type lang);
    constexpr language_iterator_type language(void) const;

private:
    language_iterator_type m_language;
};

namespace language
{
void init(void);
void init_late(void);
} // namespace language

namespace language
{
void set(language_iterator_type lang);
void reload(void);
} // namespace language

namespace language
{
language_iterator_type current(void);
language_iterator_type find(std::string_view code);
language_iterator_type const_begin(void);
language_iterator_type const_end(void);
} // namespace language

namespace language
{
std::string_view resolve(std::string_view key);
std::string resolve_gui(std::string_view key);
} // namespace language

constexpr std::string_view LanguageInfo::endonym(void) const
{
    return m_endonym;
}

constexpr std::string_view LanguageInfo::display(void) const
{
    return m_display;
}

constexpr std::string_view LanguageInfo::code(void) const
{
    return m_code;
}

constexpr const char* LanguageInfo::display_cstr(void) const
{
    return m_display.c_str();
}

constexpr language_iterator_type LanguageUpdateEvent::language(void) const
{
    return m_language;
}

#endif /* E92E5ABE_D4FD_462C_89D4_BB75774051B7 */
