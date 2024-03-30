import AfxCommon;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__)