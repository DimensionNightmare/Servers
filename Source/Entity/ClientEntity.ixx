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
	friend class UniversalMemoryPool;
	ClientEntity(World::WPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Client;

		// pDbEntity = std::make_unique<GDb::Player>();
		pDbEntity = MemPool->Allocate<GDb::Player>();
		
	}

public:
	using Ptr = std::shared_ptr<ClientEntity>;
	using CVPtr = const Ptr&;
	virtual ~ClientEntity()
	{
		
	}

	virtual void Dispose() override
	{
		Entity::Dispose();

		pDbEntity = nullptr;
	}
	
public: // dll override

	/// @brief db entity get

	GDb::Player* GetDbEntity() { return pDbEntity.get(); }

protected: // dll proxy

	uint64_t iRecordRoomId = 0;

	/// @brief db entity
	std::shared_ptr<GDb::Player> pDbEntity;

public:

	/// @brief sql table primary key
	inline static const char* SKeyName = "account_id";

};
