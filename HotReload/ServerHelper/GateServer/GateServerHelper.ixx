module;
export module GateServerHelper;

export import DNServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import FuncHelper;
import ThirdParty.PbGen;

export class GateServerHelper : public DNServer
{

private:

	GateServerHelper() = delete;
	~GateServerHelper() = default;

	GateServerHelper(const GateServerHelper&) = delete;
	// void operator=(const GateServerHelper&) = delete;

	GateServerHelper(GateServerHelper&&) = delete;
	GateServerHelper& operator=(GateServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<GateServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNServerProxyHelper::Ptr GetServerProxy() { return GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy); }

	ServerEntityManagerHelper::Ptr GetServerEntityManager() { return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager); }

	ProxyEntityManagerHelper::Ptr GetProxyEntityManager() { return GetComponent<ProxyEntityManagerHelper>(EMComponentType::ProxyEntityManager); }

	/// @brief send close to change socket
	void ServerEntityCloseEvent(Entity::Ptr entity)
	{
		// up to Global
		std::string binData;
		GMsg::g2G_RetRegistSrv request;
		request.set_server_id(entity->ID());
		request.set_is_regist(false);
		request.SerializeToString(&binData);
		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, GetClientProxy()->GetChannel());

		GetServerEntityManager()->RemoveEntity(entity->ID());
	}

	void ProxyEntityCloseEvent(Entity::Ptr entity)
	{
		ProxyEntityManagerHelper::Ptr entityMan = GetProxyEntityManager();
		uint64_t entityId = entity->ID();

		ServerEntity::Ptr serverEntity = nullptr;
		if (uint64_t serverId = entity->GetSelf<ProxyEntity>()->RecordServerId())
		{
			serverEntity = GetServerEntityManager()->GetEntity(serverId);
		}

		if (serverEntity)
		{
			std::string binData;
			GMsg::g2L_RetProxyOffline request;
			request.set_entity_id(entityId);
			request.SerializeToString(&binData);
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, serverEntity->GetSock());
		}

		entityMan->RemoveEntity(entityId);
	}
};
