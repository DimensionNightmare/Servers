module;
#include <cstdint>
export module ClientEntityHelper;

export import ClientEntity;

using namespace std;

export class ClientEntityHelper : public ClientEntity
{
private:
	ClientEntityHelper() = delete;
public:
	uint32_t& ID() { return iId; }

	uint32_t& ServerIndex() { return iServerIndex; }
};
