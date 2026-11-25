#ifndef B779E645_7AC1_4250_8292_7CEC13620351
#define B779E645_7AC1_4250_8292_7CEC13620351

namespace core
{
const std::filesystem::path& gamepath(void);
const std::filesystem::path& userpath(void);
const std::filesystem::path& modspath(void);
} // namespace core

namespace core
{
void setup_client(int argc, char** argv);
void setup_server(int argc, char** argv);
void teardown(void);
} // namespace core

#endif /* B779E645_7AC1_4250_8292_7CEC13620351 */
