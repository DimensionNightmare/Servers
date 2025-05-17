module;
export module GlobalServerHelper;

export import DNServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import FuncHelper;
import DllUtils;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

#define FUNCPLACE(func) #func, func

export class GlobalServerHelper : public DNServer
{

private:

	GlobalServerHelper() = delete;
	~GlobalServerHelper() = default;

	GlobalServerHelper(const GlobalServerHelper&) = delete;
	// void operator=(const GlobalServerHelper&) = delete;

	GlobalServerHelper(GlobalServerHelper&&) = delete;
	GlobalServerHelper& operator=(GlobalServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<GlobalServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNServerProxyHelper::Ptr GetServerProxy() { return GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy); }

	ServerEntityManagerHelper::Ptr GetServerEntityManager() { return GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager); }

	void UpdateServerGroup()
	{
		ServerEntityManagerHelper::Ptr entityMan = GetServerEntityManager();

		std::list<ServerEntity::Ptr> gates = entityMan->GetEntitysByType(EMServerType::GateServer);
		if (gates.empty())
		{
			return;
		}

		std::list<ServerEntity::Ptr> dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		std::list<ServerEntity::Ptr> logics = entityMan->GetEntitysByType(EMServerType::LogicServer);

		// alloc gate
		GMsg::COM_RetChangeCtlSrv request;
		std::string binData;

		auto registControl = [&](ServerEntity::Ptr beEntity, ServerEntity::Ptr entity)
		{
			const DNSocketProxy::Ptr& channel = entity->GetSock();
			entity->LinkNode() = beEntity;

			channel->setContextPtr(nullptr);

			// sendData
			request.set_server_ip(beEntity->ServerIp());
			request.set_server_port(beEntity->ServerPort());

			request.SerializeToString(&binData);
			// timer destory
			entity->TimerId() = TickMainSpaceDll(entityMan.get(), FUNCPLACE(&ServerEntityManager::CheckEntityCloseTimer), entity->ID());
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, channel);
			entity->SetSock(nullptr);
		};

		for (ServerEntity::Ptr gate : gates)
		{
			if (gate->HasFlag(EMServerEntityFlag::Locked))
			{
				continue;
			}

			std::list<ServerEntity::Ptr>& gatesDb = gate->GetMapLinkNode(EMServerType::DatabaseServer);
			std::list<ServerEntity::Ptr>& gatesLogic = gate->GetMapLinkNode(EMServerType::LogicServer);
			if (!dbs.empty() && gatesDb.size() < 1)
			{
				ServerEntity::Ptr ele = dbs.front();
				// dbs.pop_front();
				registControl(gate, ele);
				entityMan->UnMountEntity(ele->GetServerType(), ele);
				gatesDb.emplace_back(ele);
			}

			if (!logics.empty() && gatesLogic.size() < 1)
			{
				ServerEntity::Ptr ele = logics.front();
				// logics.pop_front();
				registControl(gate, ele);
				entityMan->UnMountEntity(ele->GetServerType(), ele);
				gatesLogic.emplace_back(ele);
			}

			if (gatesDb.size() && gatesLogic.size())
			{
				// UnMountEntity(gate->GetServerType(), it);
				gate->SetFlag(EMServerEntityFlag::Locked);
				GetLogger()->Record(ELogLevel_Debug, "Gate:{} locked!", gate->ID());
			}

		}
	}
};
