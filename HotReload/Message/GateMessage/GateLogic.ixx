module;
#include "StdAfx.h"
#include "C_Auth.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GateMessage:GateLogic;

import GateServerHelper;
import DNTask;
import Utils.StrUtils;
import MessagePack;

import Entity;
import ProxyEntityHelper;

using namespace std;
using namespace GMsg::C_Auth;
using namespace google::protobuf;
using namespace hv;
