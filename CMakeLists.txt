# CMakeList.txt: DimensionNightmareServer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 23)
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

set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP ON)

# 库目录和库头文件 Debug
include_directories("../Environment/Install/libhv/Debug/include")
include_directories("../Environment/Install/protobuf/Debug/include")
include_directories("../Environment/GameConfig/Gen/Code")

link_directories("../Environment/Install/libhv/Debug/lib")
link_directories("../Environment/Install/protobuf/Debug/lib")


project (HotReload)

file(GLOB_RECURSE srcHotReloadNormal CONFIGURE_DEPENDS
	"../Environment/GameConfig/Gen/Code/*.cc"
	"HotReload/*.cpp"
)

file(GLOB_RECURSE srcHotReloadModule CONFIGURE_DEPENDS 
	"HotReload/Source/*"
	"Source/Server/*"
	"Source/Task/*"
	"Source/Message/*"
)
add_library( HotReload SHARED)
target_sources(HotReload 
	PUBLIC 
		${srcHotReloadNormal}  
	PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES
   		${srcHotReloadModule}
)
target_compile_definitions(HotReload PRIVATE HV_STATICLIB HOTRELOAD_BUILD)
set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Runtime)
target_link_libraries(HotReload PRIVATE "hv_static.lib" "libprotobufd.lib")

if (CMAKE_VERSION VERSION_GREATER 3.12)
	set_property(TARGET HotReload PROPERTY CXX_STANDARD 23)
endif()


# DimensionNightmareServer
project (DimensionNightmareServer)
file(GLOB_RECURSE DNSourceNormal CONFIGURE_DEPENDS 
	"main.cpp"
)
file(GLOB_RECURSE DNSourceModule CONFIGURE_DEPENDS 
	"Source/*"
)

add_executable(DimensionNightmareServer)

target_sources(DimensionNightmareServer 
	PUBLIC 
		${DNSourceNormal} 
	PUBLIC FILE_SET all_my_modules TYPE CXX_MODULES FILES
		${DNSourceModule}
)
add_compile_definitions(DimensionNightmareServer HV_STATICLIB)
target_link_libraries(DimensionNightmareServer PRIVATE "hv_static.lib" "libprotobufd.lib")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DimensionNightmareServer PROPERTY CXX_STANDARD 23)
endif()

# add_executable(LibhvTest)
# file(GLOB_RECURSE srcLibhvTest CONFIGURE_DEPENDS 
# 	Test/libhv/*.ixx 
# 	../Environment/GameConfig/Gen/Code/*.cc 
# 	Source/Message/*.ixx
# )
# target_sources(LibhvTest PUBLIC ${srcLibhvTest})
# target_link_libraries(LibhvTest PRIVATE hv_static.lib libprotobufd.lib)
# if (CMAKE_VERSION VERSION_GREATER 3.12)
#   set_property(TARGET LibhvTest PROPERTY CXX_STANDARD 23)
# endif()

# TODO: 如有需要，请添加测试并安装目标。
