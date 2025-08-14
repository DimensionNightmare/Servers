cmake_minimum_required (VERSION 3.10)

if("${CMAKE_VERSION}" MATCHES "msvc")
	#https://github.com/microsoft/CMake/blob/f336929b4d90c7a5b92338dae99814ed5da4c4c9/Help/dev/experimental.rst
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
else()
	
	#https://github.com/Kitware/CMake/blob/cc2b82e2f3a96e8de78c796d63d067db68d10df6/Help/dev/experimental.rst
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
		"a9e1cf81-9932-4810-974b-6eccaf14e457" # updated value for 4.0.0
		# "d0edc3af-4c50-42ea-a356-e2862fe7a444" # 4.0.3
	)
endif()

if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	# set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Top project
project (DimensionNightmareServer LANGUAGES CXX)

message("${CMAKE_VERSION}
${CMAKE_SYSTEM_NAME}
${CMAKE_SYSTEM_VERSION}
${CMAKE_BUILD_TYPE}
${MSVC_VERSION}
${CMAKE_SYSTEM_PROCESSOR}
${CMAKE_CXX_COMPILER_ID}
")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_MODULE_STD 1)
# set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_UNITY_BUILD ON)
# find_program(CCACHE_PROGRAM ccache)
# if(CCACHE_PROGRAM)
#   message(STATUS "Set up ccache ...")
#   set(CMAKE_C_COMPILER_LAUNCHER ccache)
#   set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
# endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(STR_BUILD_TYPE ${CMAKE_BUILD_TYPE})

if (STR_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(STR_BUILD_TYPE "Release")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

set(CMAKE_PREFIX_PATH
"../Environment/Install/${STR_BUILD_TYPE}/libhv"
"../Environment/Install/${STR_BUILD_TYPE}/protobuf"
"../Environment/Install/${STR_BUILD_TYPE}/pbgen"
"../Environment/Install/${STR_BUILD_TYPE}/libpqxx"
"../Environment/Install/${STR_BUILD_TYPE}/hiredis"
"../Environment/Install/${STR_BUILD_TYPE}/redis-plus-plus"
)

find_package(libhv CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(pbgen CONFIG REQUIRED)

add_definitions(/utf-8)

################################ MainLib ################################
file(GLOB_RECURSE SourceModule CONFIGURE_DEPENDS 
	"Source/*.ixx"
)

add_library(MainSource STATIC)
target_sources(MainSource 
	PUBLIC FILE_SET CXX_MODULES TYPE CXX_MODULES FILES  
	${SourceModule}
)

target_link_libraries(MainSource PUBLIC
	hv_static
	protobuf::libprotobuf
	pbgen
	libpqxx::pqxx
	redis++::redis++_static
)

set_property(TARGET MainSource PROPERTY CXX_STANDARD 23)

################################ DimensionNightmareServer ################################
add_executable(DimensionNightmareServer)

file(GLOB_RECURSE srcProgram CONFIGURE_DEPENDS 
	"Program/*.ixx"
)

target_sources(DimensionNightmareServer 
	PRIVATE FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
	${srcProgram}
)

target_link_libraries(DimensionNightmareServer PRIVATE MainSource)

set_property(TARGET DimensionNightmareServer PROPERTY CXX_STANDARD 23)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
	set_target_properties(DimensionNightmareServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY  $<1:${CMAKE_BINARY_DIR}/>)
else()
	set_target_properties(DimensionNightmareServer PROPERTIES LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/>)
endif()

################################ HotReload ################################

file(GLOB_RECURSE srcHotReloadModule CONFIGURE_DEPENDS 
	"HotReload/*.ixx"
)

add_library( HotReload SHARED)

target_sources(HotReload 
	PRIVATE FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
	${srcHotReloadModule}
)

target_compile_definitions(HotReload PRIVATE HOTRELOAD_BUILD)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
	set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  $<1:${CMAKE_BINARY_DIR}/Runtime>)
else()
	set_target_properties(HotReload PROPERTIES LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_BINARY_DIR}/Runtime>)
endif()

target_link_libraries(HotReload PRIVATE MainSource)

################################ DNTest ################################

add_executable(DNTest)

file(GLOB_RECURSE srcDNTest CONFIGURE_DEPENDS 
	"Test/*.cpp" 
)

target_link_libraries(DNTest 
	protobuf::libprotobuf 
	pbgen 
	hv_static 
	libpqxx::pqxx
	redis++::redis++_static
)

target_sources(DNTest PRIVATE ${srcDNTest})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DNTest PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(DNTest PROPERTIES EXCLUDE_FROM_ALL TRUE)

################################ END ################################
# TODO: 如有需要，请添加测试并安装目标。

if(MSVC)

	function(generate_product_version RESULT_NAME)
		# 解析参数
		set(options)
		set(oneValueArgs
			NAME
			VERSION_MAJOR
			VERSION_MINOR
			VERSION_PATCH
			VERSION_TWEAK
			COMPANY_NAME
			FILE_DESCRIPTION
			COPYRIGHT
			TRADEMARKS
			PRODUCT_NAME
			ORIGINAL_FILENAME
			INTERNAL_NAME
		)
		set(multiValueArgs)
		cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
		
		# 设置默认值
		if(NOT ARG_VERSION_MAJOR)
			set(ARG_VERSION_MAJOR 0)
		endif()
		if(NOT ARG_VERSION_MINOR)
			set(ARG_VERSION_MINOR 0)
		endif()
		if(NOT ARG_VERSION_PATCH)
			set(ARG_VERSION_PATCH 0)
		endif()
		if(NOT ARG_VERSION_TWEAK)
			set(ARG_VERSION_TWEAK 0)
		endif()
		
		# 生成版本资源文件
		set(VERSION_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/${RESULT_NAME}.rc")
		set(VERSION_H_FILE "${CMAKE_CURRENT_BINARY_DIR}/${RESULT_NAME}.h")
		
		# 创建版本头文件
		file(WRITE ${VERSION_H_FILE}
			"#pragma once\n\n"
			"#define PRODUCT_VERSION_MAJOR ${ARG_VERSION_MAJOR}\n"
			"#define PRODUCT_VERSION_MINOR ${ARG_VERSION_MINOR}\n"
			"#define PRODUCT_VERSION_PATCH ${ARG_VERSION_PATCH}\n"
			"#define PRODUCT_VERSION_TWEAK ${ARG_VERSION_TWEAK}\n"
			"#define PRODUCT_VERSION \"${ARG_VERSION_MAJOR}.${ARG_VERSION_MINOR}.${ARG_VERSION_PATCH}.${ARG_VERSION_TWEAK}\"\n"
		)
		
			# 创建版本资源文件
		set(UTF8_BOM "")
		foreach(code 239 187 191)  # EF BB BF 的十进制表示
			string(ASCII ${code} char)
			string(APPEND UTF8_BOM "${char}")
		endforeach()

		file(WRITE ${VERSION_RC_FILE}
			"${UTF8_BOM}#include \"${RESULT_NAME}.h\"\n"
			"#include <winver.h>\n\n"
			"VS_VERSION_INFO VERSIONINFO\n"
			"FILEVERSION PRODUCT_VERSION_MAJOR, PRODUCT_VERSION_MINOR, PRODUCT_VERSION_PATCH, PRODUCT_VERSION_TWEAK\n"
			"PRODUCTVERSION PRODUCT_VERSION_MAJOR, PRODUCT_VERSION_MINOR, PRODUCT_VERSION_PATCH, PRODUCT_VERSION_TWEAK\n"
			"FILEFLAGSMASK 0x3fL\n"
			"#ifdef _DEBUG\n"
			"	FILEFLAGS VS_FF_DEBUG\n"
			"#else\n"
			"	FILEFLAGS 0x0L\n"
			"#endif\n"
			"FILEOS VOS_NT_WINDOWS32\n"
			"FILETYPE VFT_APP\n"
			"FILESUBTYPE VFT2_UNKNOWN\n"
			"BEGIN\n"
			"    BLOCK \"StringFileInfo\"\n"
			"    BEGIN\n"
			"        BLOCK \"040904b0\"\n"
			"        BEGIN\n"
		)
		
		# 添加字符串信息
		if(ARG_COMPANY_NAME)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"CompanyName\", \"${ARG_COMPANY_NAME}\"\n"
			)
		endif()
		
		if(ARG_FILE_DESCRIPTION)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"FileDescription\", \"${ARG_FILE_DESCRIPTION}\"\n"
			)
		endif()
		
		file(APPEND ${VERSION_RC_FILE}
			"            VALUE \"FileVersion\", PRODUCT_VERSION\n"
		)
		
		if(ARG_INTERNAL_NAME)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"InternalName\", \"${ARG_INTERNAL_NAME}\"\n"
			)
		endif()
		
		if(ARG_COPYRIGHT)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"LegalCopyright\", \"${ARG_COPYRIGHT}\"\n"
			)
		endif()
		
		if(ARG_ORIGINAL_FILENAME)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"OriginalFilename\", \"${ARG_ORIGINAL_FILENAME}\"\n"
			)
		endif()
		
		if(ARG_PRODUCT_NAME)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"ProductName\", \"${ARG_PRODUCT_NAME}\"\n"
			)
		endif()
		
		file(APPEND ${VERSION_RC_FILE}
			"            VALUE \"ProductVersion\", PRODUCT_VERSION\n"
		)
		
		if(ARG_TRADEMARKS)
			file(APPEND ${VERSION_RC_FILE}
				"            VALUE \"LegalTrademarks\", \"${ARG_TRADEMARKS}\"\n"
			)
		endif()
		
		# 结束块
		file(APPEND ${VERSION_RC_FILE}
			"        END\n"
			"    END\n"
			"    BLOCK \"VarFileInfo\"\n"
			"    BEGIN\n"
			"        VALUE \"Translation\", 0x409, 1200\n"
			"    END\n"
			"END\n"
		)
		
		# 设置返回变量
		set(${RESULT_NAME} ${VERSION_RC_FILE} PARENT_SCOPE)
	endfunction()

	# 为可执行文件生成版本资源
	generate_product_version(
		DimensionNightmareServer_Version
		NAME "Dimension Nightmare Server"
		VERSION_MAJOR 0
		VERSION_MINOR 0
		VERSION_PATCH 1
		VERSION_TWEAK 0
		COMPANY_NAME "Dimension Studios"
		FILE_DESCRIPTION "Dimension Nightmare Game Server"
		COPYRIGHT "Copyright © 2023 Dimension Studios. All rights reserved."
		TRADEMARKS "Dimension Nightmare™ is a trademark of Dimension Studios"
		PRODUCT_NAME "Dimension Nightmare Server"
		ORIGINAL_FILENAME "DimensionNightmareServer.exe"
		INTERNAL_NAME "DimensionNightmareServer"
	)

	target_sources(DimensionNightmareServer PRIVATE ${DimensionNightmareServer_Version})
	target_sources(HotReload PRIVATE ${DimensionNightmareServer_Version})

    # Release 模式生成 PDB
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /DEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
    # set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG")
endif()

add_custom_target(MainTarget)
add_dependencies(MainTarget MainSource HotReload)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	install(TARGETS DimensionNightmareServer DESTINATION ${CMAKE_INSTALL_PREFIX})
	
	install(TARGETS HotReload RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/Runtime)

	install(FILES 
		${PostgreSQL_LIBRARY_DIRS}/../bin/libpq.dll 
		${PostgreSQL_LIBRARY_DIRS}/../bin/libcrypto-3-x64.dll
		${PostgreSQL_LIBRARY_DIRS}/../bin/libiconv-2.dll
		${PostgreSQL_LIBRARY_DIRS}/../bin/libintl-9.dll
		${PostgreSQL_LIBRARY_DIRS}/../bin/libssl-3-x64.dll
		${PostgreSQL_LIBRARY_DIRS}/../bin/libwinpthread-1.dll
		DESTINATION ${CMAKE_INSTALL_PREFIX}/Bin)


	install(FILES Config/Server.ini  DESTINATION ${CMAKE_INSTALL_PREFIX}/Config)
	install(DIRECTORY ../Environment/GameConfig/Gen/Data DESTINATION ${CMAKE_INSTALL_PREFIX})
	
endif()
