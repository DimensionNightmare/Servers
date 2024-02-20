# CMakeList.txt: DimensionNightmareServer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# Top project
project (DimensionNightmareServer)

message("${CMAKE_VERSION} ${MSVC_VERSION} ${CMAKE_SYSTEM_VERSION}")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:${CMAKE_BUILD_TYPE},RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:${CMAKE_BUILD_TYPE},RelWithDebInfo>:ProgramDatabase>>")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:${CMAKE_BUILD_TYPE}>:${CMAKE_BUILD_TYPE}>")
endif()

# module is the experimental function. cmake experimental api needs to be enabled
if (CMAKE_VERSION EQUAL 3.25)
    set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "3c375311-a3c9-4396-a187-3227ef642046")
elseif (CMAKE_VERSION EQUAL 3.26)
    set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2182bf5c-ef0d-489a-91da-49dbc3090d2a")
elseif (CMAKE_VERSION EQUAL 3.27)
    set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "aa1f7df0-828a-4fcd-9afc-2dc80491aca7")
else()
    message(WARNING "Newer CMake version, CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API may not be set.")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Od")

set(CMAKE_PREFIX_PATH
"../Environment/Install/libhv/${CMAKE_BUILD_TYPE}/lib/cmake;"
"../Environment/Install/protobuf/${CMAKE_BUILD_TYPE}/cmake;"
"../Environment/Install/libpqxx/${CMAKE_BUILD_TYPE}/lib/cmake;"
)

find_package(libhv CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

include_directories("../Environment/Install/protobufGen/${CMAKE_BUILD_TYPE}/include")
link_directories("../Environment/Install/protobufGen/${CMAKE_BUILD_TYPE}/lib")

################################ DimensionNightmareServer ################################
file(GLOB_RECURSE DNSourceModule CONFIGURE_DEPENDS 
	"Source/*.ixx"
)

add_library(MainSource)

target_sources( MainSource
	PUBLIC FILE_SET cpp20_modules TYPE CXX_MODULES FILES ${DNSourceModule}
)

target_link_libraries(MainSource PUBLIC hv_static protobuf::libprotobuf protobufGen.lib)
set_property(TARGET MainSource PROPERTY CXX_STANDARD 23)

add_executable(DimensionNightmareServer "main.cpp")
target_link_libraries(DimensionNightmareServer PRIVATE MainSource)
set_property(TARGET DimensionNightmareServer PROPERTY CXX_STANDARD 23)

################################ HotReload ################################

file(GLOB_RECURSE srcHotReloadModule CONFIGURE_DEPENDS 
	"HotReload/Source/*.ixx"
)

add_library(HotloadSource)

target_sources(HotloadSource 
	PUBLIC FILE_SET cpp20_modules TYPE CXX_MODULES FILES  ${srcHotReloadModule}
)

target_link_libraries(HotloadSource PUBLIC MainSource  libpqxx::pqxx)

set_property(TARGET HotloadSource PROPERTY CXX_STANDARD 23)

add_library( HotReload SHARED "HotReload/HotReload.cpp")

target_compile_definitions(HotReload PUBLIC HOTRELOAD_BUILD)

set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Runtime)

target_link_libraries(HotReload PUBLIC HotloadSource)

################################ DNTest ################################

add_executable(DNTest)

file(GLOB_RECURSE srcDNTest CONFIGURE_DEPENDS 
	"Test/*.ixx" 
)

target_link_libraries(DNTest protobuf::libprotobuf protobufGen.lib hv_static libpqxx::pqxx)

target_sources(DNTest PUBLIC ${srcDNTest})

target_compile_definitions(DNTest PUBLIC -DUNICODE)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DNTest PROPERTY CXX_STANDARD 23)
endif()

# TODO: 如有需要，请添加测试并安装目标。
