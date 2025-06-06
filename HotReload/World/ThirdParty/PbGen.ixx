module;
#include <concepts>

#include "GCfg/GCfg.pb.h"
#include "GDb/GDb.pb.h"
#include "Server/S_Auth.pb.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Dedicated.pb.h"
#include "Server/S_Global.pb.h"
#include "Server/S_Gate.pb.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Logic.pb.h"

export module ThirdParty.PbGen;

import ThirdParty.Protobuf;

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
	using ::GDef_MapPointRecord;
	using ::GDef_Vector3;
	using ::GDef_MapPoint;
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

	using AccountPtr = std::shared_ptr<GDb::Account>;
	using PlayerPtr = std::shared_ptr<GDb::Player>;
	using SingleTonPtr = std::shared_ptr<GDb::SingleTon>;
}

export namespace PbGen
{

	int GetNumberFieldOptions(const FieldOptions& options, CustomFieldOptions extension)
	{
		switch(extension)
		{
			case e_primary_key:
				return options.GetExtension(ext_primary_key);
			case e_unique:
				return options.GetExtension(ext_unique);
			case e_len_limit:
				return options.GetExtension(ext_len_limit);
			case e_autogen:
				return options.GetExtension(ext_autogen);
			case e_datetime:
				return options.GetExtension(ext_datetime);
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

