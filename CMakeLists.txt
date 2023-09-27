# CMakeList.txt: DimensionNightmare 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)

# 添加库文件的define定义 防止出错
add_compile_definitions(HV_STATICLIB)

# 库目录和库头文件 Debug
include_directories(../Environment/Install/libhv/Debug/include)
include_directories(../Environment/Install/protobuf/Debug/include)
link_directories(../Environment/Install/libhv/Debug/lib)
set(PB_LIB_DIR ../Environment/Install/protobuf/Debug/lib)
file(GLOB PB_LIB_FILES CONFIGURE_DEPENDS ${PB_LIB_DIR}/*.lib)


file(GLOB_RECURSE srcDll CONFIGURE_DEPENDS 
	Source/Servers/base/*.ixx
	Source/Servers/*.ixx
	Source/*.cpp
)
project(DimensionNightmareLib)
add_library( DimensionNightmareLib SHARED ${srcDll} )
target_compile_definitions(DimensionNightmareLib PRIVATE MY_LIBRARY_EXPORTS)
target_link_libraries(DimensionNightmareLib PRIVATE hv_static.lib)
# target_compile_definitions( DimensionNightmareLib PRIVATE MY_LIBRARY_EXPORTS)
#target_include_directories( DimensionNightmareLib PUBLIC ${PROJECT_SOURCE_DIR}/Source)
#install(TARGETS DimensionNightmareLib DESTINATION ${EXECUTABLE_OUTPUT_PATH})


# DimensionNightmare
project (DimensionNightmare)
add_executable(DimensionNightmare)
file(GLOB_RECURSE srcProject CONFIGURE_DEPENDS 
	Source/**
)
target_sources(DimensionNightmare PUBLIC ${PROJECT_SOURCE_DIR}/DimensionNightmare.ixx )
target_link_libraries(DimensionNightmare PRIVATE hv_static.lib)

# 将源代码添加到此项目的可执行文件。
# add_executable(LibhvTest)
# file(GLOB_RECURSE srcLibhvTest CONFIGURE_DEPENDS Test/libhv/*.ixx)
# target_sources(LibhvTest PUBLIC ${srcLibhvTest})
# target_link_libraries(LibhvTest PRIVATE hv_static.lib)

# add_executable(GrpcTest)
# file(GLOB_RECURSE srcGrpcTest CONFIGURE_DEPENDS Test/grpc/*.ixx ../Environment/GameConfig/Gen/Code/*.cc)
# target_sources(GrpcTest PUBLIC ${srcGrpcTest})
# target_link_libraries(GrpcTest PRIVATE ${PB_LIB_FILES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DimensionNightmare PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
