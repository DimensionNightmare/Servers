module;
#include <string>

#include "google/protobuf/reflection.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/util/json_util.h"
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
#include "Common/DbExtend.pb.h"
export module ThirdParty.PbGen;

using namespace std;
using namespace google::protobuf;
using namespace GMsg;
using namespace l10n;
using namespace GCfg;
using namespace GDb;

export {
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
};

export {
	using ::ErrCode;
	using ::TipCode;
	using ::ErrText;
	using ::TipText;
	using ::L10nErr;
	using ::L10nTip;
	using ::MapPointRecord;
	using ::Vector3;
	using ::MapPoint;
};

export {
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

export {
	void PB_ShutdownProtobufLibrary() { ShutdownProtobufLibrary(); }
	auto PB_MessageToJsonString(const Message& message, std::string* output) { return util::MessageToJsonString(message, output); }
	const Descriptor* PB_FindMessageTypeByName(absl::string_view name) { return DescriptorPool::generated_pool()->FindMessageTypeByName(name); }
	const Message* PB_GetPrototype(const Descriptor* descriptor) { return MessageFactory::generated_factory()->GetPrototype(descriptor); }
	bool PB_ErrCode_IsValid(int type) { return ErrCode_IsValid(type); }
	bool PB_TipCode_IsValid(int type) { return TipCode_IsValid(type); }

	const string& PB_ErrCode_Name(int type) { return ErrCode_Name((ErrCode)type); }
	const string& PB_TipCode_Name(int type) { return TipCode_Name((TipCode)type); }
};
