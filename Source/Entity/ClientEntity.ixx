module;
#include "StdMacro.h"
export module ClientEntity;

import Entity;
import ThirdParty.PbGen;

export enum class ClientEntityFlag : uint16_t
{
	DBInited = 0,
	DBIniting,
	DBModify,
	DBModifyPartial,
	Max,
};

constexpr uint16_t ClientEntityFlagSize() { return static_cast<uint16_t>(ClientEntityFlag::Max); }

export class ClientEntity : public Entity
{

public:
	/// @brief not id's entity
	ClientEntity() : Entity(0)
	{
		eEntityType = EntityType::Client;
	}

	/// @brief mean set cliententityid
	ClientEntity(uint32_t id) : Entity(id)
	{
		eEntityType = EntityType::Client;
		pDbEntity->set_account_id(id);
	}

	virtual ~ClientEntity()
	{
		pDbEntity = nullptr;
	}
	
public: // dll override

	/// @brief get roomid
	uint32_t& RecordRoomId() { return iRecordRoomId; }

	bool HasFlag(ClientEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(ClientEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(ClientEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

	/// @brief db entity get
	Player* GetDbEntity() { return &*pDbEntity; }

protected: // dll proxy

	uint32_t iRecordRoomId = 0;

	bitset<ClientEntityFlagSize()> oFlags;

	/// @brief db entity
	unique_ptr<Player> pDbEntity = make_unique<Player>();

public:

	/// @brief sql table primary key
	inline static const char* SKeyName = "account_id";

};
