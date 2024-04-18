# CMakeList.txt: DimensionNightmareServer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.26 FATAL_ERROR)

# Top project
project (DimensionNightmareServer CXX)

message("${CMAKE_VERSION}
${MSVC_VERSION}
${CMAKE_SYSTEM_VERSION}
${CMAKE_BUILD_TYPE}
${CMAKE_SYSTEM_PROCESSOR}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

	if (MSVC_VERSION LESS 1936)
		message(FATAL_ERROR "MSVC Version < 14 not supported!")
	endif()

	# Enable Hot Reload for MSVC compilers if supported.
	if (POLICY CMP0141)
		cmake_policy(SET CMP0141 NEW)
		set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:${CMAKE_BUILD_TYPE},RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:${CMAKE_BUILD_TYPE},RelWithDebInfo>:ProgramDatabase>>")
		#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:${CMAKE_BUILD_TYPE}>:${CMAKE_BUILD_TYPE}>")
	endif()

	# module is the experimental function. cmake experimental api needs to be enabled
	if (CMAKE_VERSION EQUAL 3.26)
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
	elseif (CMAKE_VERSION EQUAL 3.27)
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "aa1f7df0-828a-4fcd-9afc-2dc80491aca7")
	elseif (CMAKE_VERSION EQUAL 3.28)
		set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "aa1f7df0-828a-4fcd-9afc-2dc80491aca7")
	else()
    	message(WARNING "Newer CMake version, CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API may not be set.")
	endif()

	set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP ON)
	
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

	if (CMAKE_CXX_COMPILER_VERSION LESS 14)
		message(FATAL_ERROR "GCC Version < 14 not supported!")
	endif()
	
endif()

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_PREFIX_PATH
"../Environment/Install/libhv/${CMAKE_BUILD_TYPE}/lib/cmake;"
"../Environment/Install/protobuf/${CMAKE_BUILD_TYPE}/cmake;"
"../Environment/Install/protobuf/${CMAKE_BUILD_TYPE}/lib/cmake;"
"../Environment/Install/libpqxx/${CMAKE_BUILD_TYPE}/lib/cmake;"
)

find_package(libhv CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

include_directories("../Environment/Install/protobufGen/${CMAKE_BUILD_TYPE}/include")
link_directories("../Environment/Install/protobufGen/${CMAKE_BUILD_TYPE}/lib")

include_directories("Source/Afx")

################################ MainLib ################################

file(GLOB_RECURSE DNSourceModule CONFIGURE_DEPENDS 
	"Source/*.ixx"
)

add_library(MainSource STATIC)
target_sources(MainSource 
	PUBLIC FILE_SET CXX_MODULES TYPE CXX_MODULES FILES  
	${DNSourceModule}
)

target_link_libraries(MainSource PUBLIC hv_static protobuf::libprotobuf protobufGen libpqxx::pqxx)

set_property(TARGET MainSource PROPERTY CXX_STANDARD 23)

################################ DimensionNightmareServer ################################
add_executable(DimensionNightmareServer)

file(GLOB_RECURSE srcProgram CONFIGURE_DEPENDS 
	"Program/*.ixx"
)

target_sources(DimensionNightmareServer 
	PUBLIC FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
	${srcProgram}
)

target_link_libraries(DimensionNightmareServer PUBLIC MainSource)

set_property(TARGET DimensionNightmareServer PROPERTY CXX_STANDARD 23)

target_compile_definitions(DimensionNightmareServer PUBLIC _${CMAKE_SYSTEM_PROCESSOR}_)


################################ HotReload ################################

file(GLOB_RECURSE srcHotReloadModule CONFIGURE_DEPENDS 
	"HotReload/*.ixx"
)

add_library( HotReload SHARED)

target_sources(HotReload 
	PUBLIC FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
	${srcHotReloadModule}
)

target_compile_definitions(HotReload PUBLIC HOTRELOAD_BUILD _${CMAKE_SYSTEM_PROCESSOR}_)

set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Runtime)

target_link_libraries(HotReload PUBLIC MainSource)

################################ DNTest ################################

add_executable(DNTest)

file(GLOB_RECURSE srcDNTest CONFIGURE_DEPENDS 
	"Test/*.cpp" 
)

target_link_libraries(DNTest protobuf::libprotobuf protobufGen hv_static libpqxx::pqxx)

target_sources(DNTest PUBLIC ${srcDNTest})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DNTest PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(DNTest PROPERTIES EXCLUDE_FROM_ALL TRUE)

# TODO: 如有需要，请添加测试并安装目标。

add_custom_target(MainTarget)
add_dependencies(MainTarget DimensionNightmareServer HotReload)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
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