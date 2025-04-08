module;
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

#include "StdMacro.h"
export module ThirdParty.PbGen;

using namespace google::protobuf;
using namespace GMsg;
using namespace l10n;
using namespace GCfg;
using namespace GDb;

export
{
	using ::Message;
	using ::Descriptor;
	using ::FieldDescriptor;
	using ::Reflection;
	using ::FieldOptions;
	using ::ext_primary_key;
	using ::ext_len_limit;
	using ::ext_unique;
	using ::ext_default;
	using ::ext_datetime;
	using ::ext_autogen;
	using ::DbModelAccount;
	using ::DbModelPlayer;
	using ::DbModelSingleTon;

	using ::ELogLevel;
	using ::EL10nType;

	using ::EL10nCode;
	using ::l10nCode;
	using ::l10nCodes;
	using ::GameDefMapPointRecord;
	using ::GameDefVector3;
	using ::GameDefMapPoint;

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
};

export namespace Common
{
	const std::string& EL10nCode_Name(EL10nCode value){ return ::EL10nCode_Name(value); }
	bool EL10nCode_IsValid(int value){ return ::EL10nCode_IsValid(value); }
	bool ELogLevel_Parse(absl::string_view name, ELogLevel* value){ return ::ELogLevel_Parse(name, value); }
	bool EL10nType_Parse(absl::string_view name, EL10nType* value){ return ::EL10nType_Parse(name, value); }
};

export namespace PBExport
{

	void ShutdownProtobufLibrary() { ::ShutdownProtobufLibrary(); }
	auto MessageToJsonString(const Message& message, std::string* output) { return json::MessageToJsonString(message, output); }
	const Descriptor* FindMessageTypeByName(absl::string_view name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }
	const Message* GetPrototype(const Descriptor* descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }
}
