module;
export module ClientEntity;

import BitFlag;
import ECSW;
import ThirdParty.PbGen;
import std.compat;

export enum class EMClientEntityFlag : uint16_t
{
	DBInited = 0,
	DBIniting,
	DBModify,
	DBModifyPartial,
	Max,
};

export class ClientEntity : public Entity, public BitFlag<EMClientEntityFlag>
{
protected:
	friend class ClientEntityManager;
	ClientEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Client;
	}

public:
	using Ptr = std::shared_ptr<ClientEntity>;
	virtual ~ClientEntity()
	{
		pDbEntity = nullptr;
	}

	virtual void Dispose() override
	{
		Entity::Dispose();
	}
	
public: // dll override

	/// @brief get roomid
	uint64_t RecordRoomId() { return iRecordRoomId; }
	void SetRecordRoomId(uint64_t roomId) { iRecordRoomId = roomId; }

	/// @brief db entity get
	GDb::PlayerPtr& GetDbEntity() { return pDbEntity; }

protected: // dll proxy

	uint64_t iRecordRoomId = 0;

	/// @brief db entity
	GDb::PlayerPtr pDbEntity;

public:

	/// @brief sql table primary key
	inline static const char* SKeyName = "account_id";

};
