module;
#include "StdMacro.h"
export module GateServerHelper;

import GateServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import ProxyEntityManagerHelper;
import FuncHelper;
import Entity;
import ThirdParty.PbGen;

export class GateServerHelper : public GateServer
{

private:

	GateServerHelper() = delete;;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }

	DNServerProxyHelper* GetSSock() { return nullptr; }

	ServerEntityManagerHelper* GetServerEntityManager() { return nullptr; }

	ProxyEntityManagerHelper* GetProxyEntityManager() { return nullptr; }

	/// @brief send close to change socket
	void ServerEntityCloseEvent(Entity* entity)
	{
		ServerEntity* cEntity = static_cast<ServerEntity*>(entity);

		// up to Global
		string binData;
		g2G_RetRegistSrv request;
		request.set_server_id(cEntity->ID());
		request.set_is_regist(false);
		request.SerializeToString(&binData);
		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, GetCSock()->GetChannel());

		GetServerEntityManager()->RemoveEntity(cEntity->ID());
	}

	void ProxyEntityCloseEvent(Entity* entity)
	{
		ProxyEntity* cEntity = static_cast<ProxyEntity*>(entity);

		ProxyEntityManagerHelper* entityMan = GetProxyEntityManager();
		uint32_t entityId = cEntity->ID();

		ServerEntity* serverEntity = nullptr;
		if (uint32_t serverId = cEntity->RecordServerId())
		{
			serverEntity = GetServerEntityManager()->GetEntity(serverId);
		}

		if (serverEntity)
		{
			string binData;
			g2L_RetProxyOffline request;
			request.set_entity_id(entityId);
			request.SerializeToString(&binData);
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, serverEntity->GetSock());
		}

		entityMan->RemoveEntity(entityId);
	}
};

static GateServerHelper* PGateServerHelper = nullptr;

export void SetGateServer(GateServer* server)
{
	PGateServerHelper = static_cast<GateServerHelper*>(server);
	ASSERT(PGateServerHelper != nullptr)
}

export GateServerHelper* GetGateServer()
{
	return PGateServerHelper;
}
