#pragma once

#include "Common/Common.pb.h"

import Logger;
import Config.Server;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__)
