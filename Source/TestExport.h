#pragma once

#ifdef MY_LIBRARY_EXPORTS
#define MY_LIBRARY_API __declspec(dllexport)
#else
#define MY_LIBRARY_API __declspec(dllimport)
#endif




#ifdef __cplusplus
extern "C" {
#endif

MY_LIBRARY_API int ServerStart();
MY_LIBRARY_API int ServerStop();

#ifdef __cplusplus
}
#endif