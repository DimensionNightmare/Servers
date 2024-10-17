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
	using ::Account;
	using ::Player;
	using ::SingleTon;

	using ::ErrCode;
	using ::TipCode;
	using ::l10nErr;
	using ::l10nTip;
	using ::l10nErrs;
	using ::l10nTips;
	using ::MapPointRecord;
	using ::Vector3;
	using ::MapPoint;

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

export namespace PBExport
{
	void ShutdownProtobufLibrary() { ::ShutdownProtobufLibrary(); }
	auto MessageToJsonString(const Message& message, std::string* output) { return json::MessageToJsonString(message, output); }
	const Descriptor* FindMessageTypeByName(absl::string_view name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }
	const Message* GetPrototype(const Descriptor* descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }
	bool ErrCode_IsValid(int type) { return ::ErrCode_IsValid(type); }
	bool TipCode_IsValid(int type) { return ::TipCode_IsValid(type); }

	const string& ErrCode_Name(int type) { return ::ErrCode_Name((ErrCode)type); }
	const string& TipCode_Name(int type) { return ::TipCode_Name((TipCode)type); }
};
