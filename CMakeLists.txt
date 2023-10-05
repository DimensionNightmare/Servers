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
include_directories(../Environment/Install/libhv/Debug/include)
link_directories(../Environment/Install/libhv/Debug/lib)
include_directories(../Environment/Install/protobuf/Debug/include)
set(PB_LIB_DIR ../Environment/Install/protobuf/Debug/lib)

file(GLOB PB_LIB_FILES CONFIGURE_DEPENDS ${PB_LIB_DIR}/*.lib)

project (HotReload)

file(GLOB_RECURSE srcHotReload CONFIGURE_DEPENDS 
	HotReload/*
)
add_library( HotReload SHARED ${srcHotReload} )
add_compile_definitions(HotReload PRIVATE HV_STATICLIB)
target_compile_definitions(HotReload PRIVATE HOTRELOAD_BUILD)
target_link_libraries(HotReload PRIVATE hv_static.lib)
set_target_properties(HotReload PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/Runtime)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET HotReload PROPERTY CXX_STANDARD 20)
endif()


# DNServer
project (DNServer)
add_executable(DNServer)
file(GLOB_RECURSE srcDn CONFIGURE_DEPENDS 
  main.ixx
	Source/*
)

target_sources(DNServer PUBLIC ${srcDn})
add_compile_definitions(DNServer HV_STATICLIB)
target_link_libraries(DNServer PRIVATE hv_static.lib)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DNServer PROPERTY CXX_STANDARD 20)
endif()

# 将源代码添加到此项目的可执行文件。
add_executable(LibhvTest)
file(GLOB_RECURSE srcLibhvTest CONFIGURE_DEPENDS Test/libhv/*.ixx)
target_sources(LibhvTest PUBLIC ${srcLibhvTest})
target_link_libraries(LibhvTest PRIVATE hv_static.lib)

add_executable(GrpcTest)
file(GLOB_RECURSE srcGrpcTest CONFIGURE_DEPENDS Test/grpc/*.ixx ../Environment/GameConfig/Gen/Code/*.cc)
target_sources(GrpcTest PUBLIC ${srcGrpcTest})
target_link_libraries(GrpcTest PRIVATE ${PB_LIB_FILES})



# TODO: 如有需要，请添加测试并安装目标。
