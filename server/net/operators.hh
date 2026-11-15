#ifndef F0A3C751_9D64_4E28_B7A1_5C2E8D9046F3
#define F0A3C751_9D64_4E28_B7A1_5C2E8D9046F3

#include "shared/net/ed25519.hh"

namespace operators
{
void init(void);
void shutdown(void);
} // namespace operators

namespace operators
{
void add(const ed25519::pkey_type& pkey);
void remove(const ed25519::pkey_type& pkey);
bool contains(const ed25519::pkey_type& pkey);
} // namespace operators

#endif /* F0A3C751_9D64_4E28_B7A1_5C2E8D9046F3 */
