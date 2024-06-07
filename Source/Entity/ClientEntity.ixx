module;
#include <cstdint>
#include <bitset>
#include "pqxx/transaction"

#include "GDef/GDef.pb.h"
export module ClientEntity;

import Entity;

using namespace std;
using namespace GDb;

export enum class ClientEntityFlag : uint16_t
{
	DBInited = 0,
	DBIniting,
	DBModify,
	Max,
};

constexpr uint16_t ClientEntityFlagSize() { return static_cast<uint16_t>(ClientEntityFlag::Max); }

class ClientEntityManager;

export class ClientEntity : public Entity
{
	friend class ClientEntityManager;
public:
	ClientEntity();
	ClientEntity(uint32_t id);
	virtual ~ClientEntity();

public: // dll override
	uint32_t& ServerIndex() { return iServerIndex; }

	bool HasFlag(ClientEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(ClientEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(ClientEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

protected: // dll proxy
	uint32_t iServerIndex = 0;

	bitset<ClientEntityFlagSize()> oFlags;

	unique_ptr<Player> pDbPlayer;
};

ClientEntity::ClientEntity() : Entity(0)
{
	eEntityType = EntityType::Client;
}

ClientEntity::ClientEntity(uint32_t id) : Entity(id)
{
	eEntityType = EntityType::Client;
}

ClientEntity::~ClientEntity()
{
	pDbPlayer = nullptr;
}
