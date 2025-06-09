module;
export module ClientEntity;

import BitFlag;
import ECSW;
import ThirdParty.Protobuf;
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
	using CVPtr = const Ptr&;
	virtual ~ClientEntity()
	{
		pDbEntity = nullptr;
	}

	virtual void Dispose() override
	{
		Entity::Dispose();
	}
	
public: // dll override

	/// @brief db entity get

	template <typename T = Message>
	std::shared_ptr<T> GetDbEntity() { return std::static_pointer_cast<T>(pDbEntity); }

protected: // dll proxy

	uint64_t iRecordRoomId = 0;

	/// @brief db entity
	std::shared_ptr<Message> pDbEntity;

public:

	/// @brief sql table primary key
	inline static const char* SKeyName = "account_id";

};
