module;
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

export module ControlMessage:ControlGlobal;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::GlobalControl;

