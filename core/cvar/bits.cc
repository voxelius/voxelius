#include "core/pch.hh"

#include "core/cvar/bits.hh"

#include "core/utils/string.hh"

constexpr static char UNSET_CHAR = '-';

static char flag_char(cvar::flag_bits bit)
{
    switch(bit) {
        case FCVAR_ADMIN:
            return 'O';

        case FCVAR_NOTIFY:
            return 'N';

        case FCVAR_READONLY:
            return 'R';

        case FCVAR_ARCHIVE:
            return 'A';

        case FCVAR_REPLICATED:
            return 'S';

        case FCVAR_HIDDEN:
            return 'H';

        case FCVAR_RECALL:
            return 'L';
    }

    return '?';
}

void cvar::make_bitstring_short(flag_bits bits, std::string& out)
{
    constexpr static std::size_t bit_count = 8 * sizeof(flag_bits);

    out.clear();

    for(std::size_t i = 0; i < bit_count; ++i) {
        auto bit = static_cast<flag_bits>(1 << (bit_count - i - 1));

        if(bits & bit) {
            out.push_back(flag_char(bit));
        }
        else {
            out.push_back(UNSET_CHAR);
        }
    }
}

void cvar::make_bitstring_long(flag_bits bits, std::string& out)
{
    thread_local std::vector<std::string> values;

    values.clear();

    if(bits & FCVAR_REPLICATED) {
        values.push_back("replicated");
    }

    if(bits & FCVAR_READONLY) {
        values.push_back("readonly");
    }

    if(bits & FCVAR_HIDDEN) {
        values.push_back("hidden");
    }

    if(bits & FCVAR_ARCHIVE) {
        values.push_back("archive");
    }

    if(bits & FCVAR_NOTIFY) {
        values.push_back("notify");
    }

    if(bits & FCVAR_ADMIN) {
        values.push_back("admin");
    }

    if(bits & FCVAR_RECALL) {
        values.push_back("recall");
    }

    if(values.size()) {
        out = utils::join(values, " ");
    }
    else {
        out.clear();
    }
}
