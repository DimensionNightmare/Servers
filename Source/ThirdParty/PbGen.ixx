module;
#include <concepts>

#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/extension_set.h"

#include "GCfg/GCfg.pb.h"
#include "l10n/l10n.pb.h"
#include "GDb/GDb.pb.h"
#include "Server/S_Auth.pb.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Dedicated.pb.h"
#include "Server/S_Global.pb.h"
#include "Server/S_Gate.pb.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Logic.pb.h"

export module ThirdParty.PbGen;

// make_wrapper using  to export static Function

template <typename F>
concept NoArgCallable = requires(F f) {
    { std::invoke(f) } -> std::same_as<void>;
};

template <NoArgCallable F>
auto make_wrapper(F&& f) {
    return [f=std::forward<F>(f)]() { 
        f(); 
    };
}

template <typename F>
auto make_wrapper(F&& f) requires (!NoArgCallable<F>) {
    return [f=std::forward<F>(f)](auto&&... args) -> decltype(auto) {
        return f(std::forward<decltype(args)>(args)...);
    };
}

export
{
	using namespace google::protobuf;

	using ::Message;
	using ::Descriptor;
	using ::FieldDescriptor;
	using ::Reflection;
	using ::FieldOptions;
	
	using ::ELogLevel;
	using ::EL10nType;

	using ::EL10nCode;
	using ::GDef_MapPointRecord;
	using ::GDef_Vector3;
	using ::GDef_MapPoint;

	using ::EL10nCode_IsValid;
	using ::ELogLevel_Parse;
	using ::EL10nType_Parse;

	using ::ShutdownProtobufLibrary;
	using ::json::MessageToJsonString;

	enum CustomFieldOptions
	{
		e_primary_key = 1,
		e_len_limit = 2,
		e_unique = 3,
		e_default = 4,
		e_datetime = 5,
		e_autogen = 6,
	};
}

export namespace l10n
{

	using l10n::l10nCode;
	using l10n::l10nCodes;
}


export namespace GMsg
{
	using GMsg::COM_RetHeartbeat;
	using GMsg::COM_ReqRegistSrv;
	using GMsg::COM_ResRegistSrv;
	using GMsg::COM_RetChangeCtlSrv;

	// DB <-> Logic
	using GMsg::L2D_ReqLoadData;
	using GMsg::D2L_ResLoadData;
	using GMsg::L2D_ReqSaveData;
	using GMsg::D2L_ResSaveData;

	// DS <-> Logic
	using GMsg::d2L_ReqLoadEntityData;
	using GMsg::L2d_ResLoadEntityData;
	using GMsg::d2L_ReqSaveEntityData;
	using GMsg::L2d_ResSaveEntityData;
	using GMsg::d2L_ReqRegistSrv;

	// Gate <-> Logic
	using GMsg::g2L_RetProxyOffline;

	// Auth <-> Gate
	using GMsg::A2g_ReqAuthAccount;
	using GMsg::g2A_ResAuthAccount;
	
	// Global <-> Gate
	using GMsg::g2G_RetRegistSrv;
	using GMsg::g2G_RetRegistChild;

	// Server <-> Client
	using GMsg::C2S_ReqAuthToken;
	using GMsg::S2C_ResAuthToken;
	using GMsg::S2C_RetAccountReplace;
}

export namespace GDb
{
	using GDb::Account;
	using GDb::Player;
	using GDb::SingleTon;
}

export namespace PbGen
{

	auto EL10nCode_Name_(EL10nCode param){return EL10nCode_Name(param);}

	auto FindMessageTypeByName(auto name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }

	auto GetPrototype(auto descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }

	int GetNumberFieldOptions(const FieldOptions& options, CustomFieldOptions extension)
	{
		switch(extension)
		{
			case e_primary_key:
				return options.GetExtension(ext_primary_key);
			case e_unique:
				return options.GetExtension(ext_unique);
			case e_len_limit:
				return options.GetExtension(ext_datetime);
			case e_autogen:
				return options.GetExtension(ext_len_limit);
			case e_datetime:
				return options.GetExtension(ext_autogen);
			default:
				throw std::invalid_argument("Invalid extension type.");
		}
	}

	auto GetStringFieldOptions(const FieldOptions& options, CustomFieldOptions extension)
	{
		switch(extension)
		{
			case e_default:
				return options.GetExtension(ext_default);
			default:
				throw std::invalid_argument("Invalid extension type.");
		}
	}

}

