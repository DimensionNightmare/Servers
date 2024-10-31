module;
#include "StdMacro.h"
export module ClientEntity;

import Entity;
import ThirdParty.PbGen;

export enum class EMClientEntityFlag : uint16_t
{
	DBInited = 0,
	DBIniting,
	DBModify,
	DBModifyPartial,
	Max,
};

constexpr uint16_t ClientEntityFlagSize() { return static_cast<uint16_t>(EMClientEntityFlag::Max); }

export class ClientEntity : public Entity
{

public:
	/// @brief not id's entity
	ClientEntity() : Entity(0)
	{
		eEntityType = EMEntityType::Client;
	}

	/// @brief mean set cliententityid
	ClientEntity(uint32_t id) : Entity(id)
	{
		eEntityType = EMEntityType::Client;
		pDbEntity->set_account_id(id);
	}

	virtual ~ClientEntity()
	{
		pDbEntity = nullptr;
	}
	
public: // dll override

	/// @brief get roomid
	uint32_t& RecordRoomId() { return iRecordRoomId; }

	bool HasFlag(EMClientEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(EMClientEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(EMClientEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

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
