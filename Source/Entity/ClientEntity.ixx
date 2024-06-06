module;
#include <cstdint>
#include <bitset>

#include "GDef/GDef.pb.h"
export module ClientEntity;

import Entity;

using namespace std;
using namespace GDb;

export enum class ClientEntityFlag : uint16_t
{
	DBInited = 0,
	DBIniting,
	Max,
};

constexpr uint16_t ClientEntityFlagSize() { return static_cast<uint16_t>(ClientEntityFlag::Max); }

export class ClientEntity : public Entity
{
public:
	ClientEntity();
	ClientEntity(uint32_t id);
	virtual ~ClientEntity();

public: // dll override
	void Load();
	void Load(const string& dbData);

	void Save();

	uint32_t& ServerIndex() { return iServerIndex; }

	bool HasFlag(ClientEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(ClientEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(ClientEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

protected: // dll proxy
	uint32_t iServerIndex = 0;

	bitset<ClientEntityFlagSize()> oFlags;

	unique_ptr<PropertyEntity> pPropertyEntity;
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
	Save();

	pPropertyEntity = nullptr;
}

void ClientEntity::Load(const string& dbData)
{
	string bitString;
	if (bitString.length() != ClientEntityFlagSize())
	{
		if (bitString.length() < ClientEntityFlagSize())
		{

			bitString.insert(bitString.begin(), ClientEntityFlagSize() - bitString.length(), '0');
		}
		else
		{
			bitString = bitString.substr(bitString.length() - ClientEntityFlagSize());
		}
	}

	oFlags = bitset<ClientEntityFlagSize()>(bitString);
}

void ClientEntity::Save()
{
}
