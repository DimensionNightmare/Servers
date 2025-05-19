module;
export module ClientEntity;

import ECSW;
import ThirdParty.PbGen;


export enum class EMClientEntityFlag : uint16_t
{
	DBInited = 0,
	DBIniting,
	DBModify,
	DBModifyPartial,
	Max,
};

using ClientEntityBitFlag = std::bitset<static_cast<uint16_t>(EMClientEntityFlag::Max)>;

export class ClientEntity : public Entity
{
protected:
	friend class ClientEntityManagerHelper;
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

	bool HasFlag(EMClientEntityFlag flag) { return oFlags.test(uint16_t(flag)); }
	void SetFlag(EMClientEntityFlag flag) { oFlags.set(uint16_t(flag)); }
	void ClearFlag(EMClientEntityFlag flag) { oFlags.reset(uint16_t(flag)); }

	/// @brief db entity get
	GDb::Player* GetDbEntity() { return &*pDbEntity; }

protected: // dll proxy

	uint64_t iRecordRoomId = 0;

	ClientEntityBitFlag oFlags;

	/// @brief db entity
	std::unique_ptr<GDb::Player> pDbEntity = std::make_unique<GDb::Player>();

public:

	/// @brief sql table primary key
	inline static const char* SKeyName = "account_id";

};
