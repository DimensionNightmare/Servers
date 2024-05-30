cmake_minimum_required (VERSION 3.26 FATAL_ERROR)

# Top project
project (DimensionNightmareServer)

message("${CMAKE_VERSION}
${CMAKE_SYSTEM_NAME}
${CMAKE_SYSTEM_VERSION}
${CMAKE_BUILD_TYPE}
${CMAKE_SYSTEM_PROCESSOR}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set(CMAKE_UNITY_BUILD ON)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "Set up ccache ...")
  set(CMAKE_C_COMPILER_LAUNCHER ccache)
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(CMAKE_PREFIX_PATH
"../Environment/Install/libhv/${CMAKE_BUILD_TYPE}"
"../Environment/Install/protobuf/${CMAKE_BUILD_TYPE}"
"../Environment/Install/pbgen/${CMAKE_BUILD_TYPE}"
"../Environment/Install/libpqxx/${CMAKE_BUILD_TYPE}"
"../Environment/Install/hiredis/${CMAKE_BUILD_TYPE}"
"../Environment/Install/redis-plus-plus/${CMAKE_BUILD_TYPE}"
)

find_package(libhv CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(pbgen CONFIG REQUIRED)

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

target_link_libraries(MainSource PUBLIC
	hv_static
	protobuf::libprotobuf
	pbgen
	libpqxx::pqxx
	redis++::redis++_static
)

target_compile_definitions(MainSource PRIVATE _${CMAKE_SYSTEM_PROCESSOR}_ MAINLIB_BUILD)

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

target_compile_definitions(DimensionNightmareServer PRIVATE _${CMAKE_SYSTEM_PROCESSOR}_)

################################ HotReload ################################

file(GLOB_RECURSE srcHotReloadModule CONFIGURE_DEPENDS 
	"HotReload/*.ixx"
)

add_library( HotReload SHARED)
# add_library( HotReload SHARED "HotReload/HotReload.cpp")

target_sources(HotReload 
	PRIVATE FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
	${srcHotReloadModule}
)

target_compile_definitions(HotReload PRIVATE HOTRELOAD_BUILD _${CMAKE_SYSTEM_PROCESSOR}_)

if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
	set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Runtime)
else()
	set_target_properties(HotReload PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Runtime)
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

# TODO: 如有需要，请添加测试并安装目标。

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