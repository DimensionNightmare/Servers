# CMakeList.txt: DimensionNightmare 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (DimensionNightmare)

# 库目录和库头文件
include_directories(Source/Plugin/libhv/include)
link_directories(Source/Plugin/libhv/lib)

# 添加库文件的define定义 防止出错
add_compile_definitions(HV_STATICLIB)

# 将源代码添加到此项目的可执行文件。
add_executable(DimensionNightmare)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS DimensionNightmare.ixx)
target_sources(DimensionNightmare PUBLIC ${sources})

# 添加链接的库文件
target_link_libraries(DimensionNightmare PRIVATE hv_static.lib)

# 将源代码添加到此项目的可执行文件。
add_executable(Test)
file(GLOB_RECURSE sourcesTest CONFIGURE_DEPENDS "Test/*.ixx")
target_sources(Test PUBLIC ${sourcesTest})

# 添加链接的库文件
target_link_libraries(Test PRIVATE hv_static.lib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DimensionNightmare PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
