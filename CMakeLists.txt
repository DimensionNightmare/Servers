# CMakeList.txt: DNServer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 库目录和库头文件 Debug
include_directories("../Environment/Install/libhv/Debug/include")
include_directories("../Environment/Install/protobuf/Debug/include")
include_directories("../Environment/GameConfig/Gen/Code")

link_directories("../Environment/Install/libhv/Debug/lib")
link_directories("../Environment/Install/protobuf/Debug/lib")

project (HotReload)

file(GLOB_RECURSE srcHotReload CONFIGURE_DEPENDS 
	"HotReload/*"
	"Source/Server/*"
	"../Environment/GameConfig/Gen/Code/*.cc"
)
add_library( HotReload SHARED ${srcHotReload} )
target_compile_definitions(HotReload PRIVATE HV_STATICLIB HOTRELOAD_BUILD)
set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Runtime)
target_link_libraries(HotReload PRIVATE "hv_static.lib" "libprotobufd.lib")

if (CMAKE_VERSION VERSION_GREATER 3.12)
	set_property(TARGET HotReload PROPERTY CXX_STANDARD 23)
endif()


# DNServer
project (DNServer)
add_executable(DNServer)
file(GLOB_RECURSE srcDn CONFIGURE_DEPENDS 
	"Source/*"
	"../Environment/GameConfig/Gen/Code/*.cc"
	"main.ixx"
)
target_sources(DNServer PUBLIC ${srcDn})
add_compile_definitions(DNServer HV_STATICLIB)
target_link_libraries(DNServer PRIVATE "hv_static.lib" "libprotobufd.lib")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DNServer PROPERTY CXX_STANDARD 23)
endif()

add_executable(LibhvTest)
file(GLOB_RECURSE srcLibhvTest CONFIGURE_DEPENDS 
	Test/libhv/*.ixx 
	../Environment/GameConfig/Gen/Code/*.cc 
	Source/Message/*.ixx
)
target_sources(LibhvTest PUBLIC ${srcLibhvTest})
target_link_libraries(LibhvTest PRIVATE hv_static.lib libprotobufd.lib)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET LibhvTest PROPERTY CXX_STANDARD 23)
endif()

# TODO: 如有需要，请添加测试并安装目标。
