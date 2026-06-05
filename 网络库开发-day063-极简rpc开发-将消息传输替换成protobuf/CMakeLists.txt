cmake_minimum_required(VERSION 3.22)
project(RpcServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 pthread 支持
find_package(Threads REQUIRED)

# 如果使用 nlohmann/json，可以尝试查找（如果系统已安装）
# 如果没有安装，可以手动包含头文件路径
# 假设你把 json.hpp 放在了 /usr/include/nlohmann/ 或者项目根目录的 include 文件夹
# 下面假设已通过 apt 安装（头文件在 /usr/include/nlohmann）
# 如果你的 json.hpp 在项目根目录的 include 下，可以取消注释下一行
# include_directories(include)

# 只搜索 src 目录下的 .cpp 文件（递归）
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 生成可执行文件
add_executable(echo_server ${SOURCES})

# 链接线程库
target_link_libraries(echo_server Threads::Threads)

# 可选：如果需要调试，可以设置编译选项
target_compile_options(echo_server PRIVATE -g -O0)
