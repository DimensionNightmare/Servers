module;
#include <concepts>

#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/json/json.h"
#include "google/protobuf/extension_set.h"

#include "GCfg/GCfg.pb.h"
#include "GDef/GDef.pb.h"
#include "l10n/l10n.pb.h"
#include "Common/Common.pb.h"
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
	using ::GameDefMapPointRecord;
	using ::GameDefVector3;
	using ::GameDefMapPoint;

	using ::EL10nCode_IsValid;
	using ::ELogLevel_Parse;
	using ::EL10nType_Parse;

	using ::ShutdownProtobufLibrary;
	using ::json::MessageToJsonString;
}

export
{
	using namespace l10n;

	using ::l10nCode;
	using ::l10nCodes;
}

export
{
	using namespace GMsg;

	using ::L2D_ReqLoadData;
	using ::D2L_ResLoadData;
	using ::d2L_ReqLoadEntityData;
	using ::L2d_ResLoadEntityData;
	using ::COM_RetHeartbeat;
	using ::A2g_ReqAuthAccount;
	using ::g2A_ResAuthAccount;

	using ::COM_ReqRegistSrv;
	using ::COM_ResRegistSrv;
	using ::g2A_ResAuthAccount;
	using ::L2D_ReqSaveData;
	using ::D2L_ResSaveData;
	using ::d2L_ReqSaveEntityData;
	using ::L2d_ResSaveEntityData;

	using ::COM_RetChangeCtlSrv;
	using ::g2L_RetProxyOffline;
	using ::g2G_RetRegistSrv;
	using ::C2S_ReqAuthToken;
	using ::S2C_ResAuthToken;
	using ::S2C_RetAccountReplace;
	using ::g2G_RetRegistChild;
	using ::d2L_ReqRegistSrv;
}

export
{
	using namespace GDb;

	using ::Account;
	using ::Player;
	using ::SingleTon;
}

export
{
	auto EL10nCode_Name_(EL10nCode param){return EL10nCode_Name(param);}

	auto FindMessageTypeByName(auto name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }
	auto GetPrototype(auto descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }

	enum CustomFieldOptions
	{
		e_primary_key = 1,
		e_len_limit = 2,
		e_unique = 3,
		e_default = 4,
		e_datetime = 5,
		e_autogen = 6,
	};

	
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

