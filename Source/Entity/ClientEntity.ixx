export module ClientEntity;

import BitFlag;
import ECSW;
import ThirdParty.PbGen;
import std.compat;

export enum class EMClientEntityFlag : uint16_t
{
	None = 0,
	DBInited,
	DBIniting,
	DBModify,
	DBModifyPartial,
	Max,
};

export class ClientEntity : public Entity, public BitFlag<EMClientEntityFlag>
{
protected:
	friend class UniversalMemoryPool;
	ClientEntity(World::CVPtr world):Entity(world)
	{
		eEntityType = EMEntityType::Client;

		pDbEntity = P_InstanceHolder->GetMemPool().Allocate<GDb::Player>();
		
	}

public:
	using Ptr = std::shared_ptr<ClientEntity>;
	using CVPtr = const Ptr&;
	virtual ~ClientEntity()
	{
		
	}

	virtual void Dispose() override
	{
		pDbEntity = nullptr;
		
		Entity::Dispose();
	}
	
public: 

	/// @brief db entity get

	GDb::Player* GetDbEntity() { return pDbEntity.get(); }

protected: // dll proxy

	size_t iRecordRoomId = 0;

	/// @brief db entity
	std::shared_ptr<GDb::Player> pDbEntity;
};
