export module ApiManager:ApiDevelopment;

import Server;
import WebProxyHelper;
import Logger;
import Task;
import ThirdParty.PbGen;
import AuthServerHelper;

#define MSGSET writer->response->SetBody


export void ApiDevelopment(Server::Ptr server)
{
	WebProxyHelper::Ptr webProxyHelper = server->GetComponent<WebProxyHelper>(EMComponentType::WebProxy);

	webProxyHelper->service->POST("/Develop/Server/LogicRandInfo", [server](hv::HttpRequestPtr req, hv::HttpResponseWriterPtr writer) ->TaskVoid
		{
			nlohmann::json retData;

			AuthServerHelper::Ptr dnServer = server->GetSelf<AuthServerHelper>();
			ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

			GMsg::A2g_ReqLogicServerIp request;
			GMsg::g2A_ResLogicServerIp response;
			bool success = co_await clientProxy->AddMsg(EMMsgDeal::Redir, &request, &response);

			if(success)
			{
				std::string binData;
				auto state = MessageToJsonString(response, &binData);
				retData["Data"] = nlohmann::json::parse(binData);
			}
			else
			{

			}

			MSGSET(retData.dump());
			writer->End();
			co_return;
		});
}
