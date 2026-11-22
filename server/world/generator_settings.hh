#ifndef B7C4A1D2_E3F5_4A6B_8C9D_0E1F2A3B4C5D
#define B7C4A1D2_E3F5_4A6B_8C9D_0E1F2A3B4C5D

namespace generator_settings
{
bool read_number(lua_State* L, int table_idx, const char* generator, const char* key, float& out);
bool read_integer(lua_State* L, int table_idx, const char* generator, const char* key, lua_Integer& out);
bool read_table(lua_State* L, int table_idx, const char* generator, const char* key, int& index);
bool check_keys(lua_State* L, int table_idx, const char* generator, std::span<const char* const> allowed);
} // namespace generator_settings

#endif /* B7C4A1D2_E3F5_4A6B_8C9D_0E1F2A3B4C5D */
