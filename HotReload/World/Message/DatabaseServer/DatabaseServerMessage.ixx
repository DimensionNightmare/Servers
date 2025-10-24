export module DatabaseServerMessage;

import :DatabaseCommon;
// import :DatabaseGate;

import ThirdParty.Protobuf;
import MessageRegister;
import StrUtils;
import MessagePack;
import std.compat;

export class DatabaseServerMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
		#define MSG_MAPPING(map, msg, func) \
			map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
			make_pair(msg::internal_default_instance(), func))


		// MSG_MAPPING(MHandleMap, GMsg::L2D_ReqLoadData, &DatabaseServerMessage::Exe_ReqLoadData);
		// MSG_MAPPING(MHandleMap, GMsg::L2D_ReqSaveData, &DatabaseServerMessage::Exe_ReqSaveData);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetChangeCtlSrv, &DatabaseServerMessage::Exe_RetChangeCtlSrv);

		#undef MSG_MAPPING
	}

	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return &DatabaseServerMessage::Evt_ReqRegistSrv;
	}

	inline static std::shared_ptr<DatabaseServerMessageHandle> PInstance;
};


export namespace DatabaseServerMessage
{

	export template<typename MsgReq, typename MsgRes, EMMsgDeal MsgDeal, bool IsTemp = false>
	class MessageRegistry
	{
	public:
		MessageRegistry(std::function<void(MsgReq&, MsgRes&, SocketChannel::Ptr)> funcExector)
		{

			oExector = funcExector;

			if(IsTemp == true)
			{
				if(DatabaseServerMessageHandle::PInstance == nullptr)
				{
					DatabaseServerMessageHandle::PInstance = std::make_shared<DatabaseServerMessageHandle>();
				}

				size_t msgHash = DoStringHash(MsgReq::GetDescriptor()->full_name());
				switch(MsgDeal)
				{
					case EMMsgDeal::Req:
						DatabaseServerMessageHandle::PInstance->MHandleMap.emplace(msgHash, std::make_pair(MsgReq::internal_default_instance(), [this](auto a, auto b, auto c){ TickMessage(a,b,c); }));
						break;
					case EMMsgDeal::Ret:
						DatabaseServerMessageHandle::PInstance->MHandleRetMap.emplace(msgHash, std::make_pair(MsgReq::internal_default_instance(), [this](auto a, auto b){ TickMessage(a,b); }));
						break;
					case EMMsgDeal::Redir:
						DatabaseServerMessageHandle::PInstance->MHandleRedirectMap.emplace(msgHash, std::make_pair(MsgReq::internal_default_instance(), [this](auto a, auto b, auto c){ TickMessage(a,b,c); }));
						break;
					
				}
			}

			
		}

		void TickMessage(SocketChannel::CVPtr channel, const std::string& binMsg)
		{
			MessageRegistry<MsgReq, MsgRes, MsgDeal, false> exector(oExector);

			exector.pChannel = channel;

			if(!exector.oMsgReq.ParseFromString(binMsg))
			{
				return;
			}

			exector.bIsReply = true;

			exector.Exector();
		}

		void TickMessage(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
		{
			MessageRegistry<MsgReq, MsgRes, MsgDeal, false> exector(oExector);

			exector.pChannel = channel;
			exector.iMsgId = msgId;
			
			if(!exector.oMsgReq.ParseFromString(binMsg))
			{
				exector.oMsgRes.set_errorcode(::EL10nCode_MsgParse); //EL10nCode_MsgParse
				return;
			}

			exector.Exector();
		}

		void Exector()
		{
			oExector(oMsgReq, oMsgRes, pChannel);
		}
		
		virtual ~MessageRegistry()
		{
			if(IsTemp)
			{
				return;
			}

			if(!bIsReply)
			{
				Reply();
			}
		}

		virtual void Reply()
		{
			bIsReply = true;

			std::string binData;
			oMsgRes.SerializeToString(&binData);
			MessagePackAndSend(iMsgId, EMMsgDeal::Res, binData, pChannel);
		}

	public:
		
		MsgReq oMsgReq;
		MsgRes oMsgRes;
		SocketChannel::Ptr pChannel;
		size_t iMsgId = 0;
		bool bIsReply = false;
		std::function<void(MsgReq&, MsgRes&, SocketChannel::Ptr)> oExector;
	};

}