#pragma once

#include "Common/Common.pb.h"

import Logger;
import Config.Server;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__)

#define MSG_MAPPING(map, msg, func) \
	map.emplace( hashStr(msg::GetDescriptor()->full_name()), \
	make_pair(msg::internal_default_instance(), func))

