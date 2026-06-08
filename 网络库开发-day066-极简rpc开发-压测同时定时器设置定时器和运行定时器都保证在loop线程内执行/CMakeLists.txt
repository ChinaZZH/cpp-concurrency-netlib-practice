cmake_minimum_required(VERSION 3.22)
project(RpcServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加 pthread 支持
find_package(Threads REQUIRED)

# 查找 添加 Protobuf 库
find_package(Protobuf REQUIRED)


# 假设 .proto 文件放在项目根目录下的 proto/ 文件夹
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")

# 输出目录
set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})


# proto 生成 C++ 代码
foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GENERATED_SRC ${PROTO_OUT_DIR}/${PROTO_NAME}.pb.cc)
    set(GENERATED_HDR ${PROTO_OUT_DIR}/${PROTO_NAME}.pb.h)
    add_custom_command(
        OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS --cpp_out=${PROTO_OUT_DIR} --proto_path=${PROTO_DIR} ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating ${PROTO_NAME}.pb.cc/.h in ${PROTO_OUT_DIR}"
    )
    list(APPEND PROTO_SRCS ${GENERATED_SRC})
    list(APPEND PROTO_HDRS ${GENERATED_HDR})
endforeach()

# 如果使用 nlohmann/json，可以尝试查找（如果系统已安装）
# 如果没有安装，可以手动包含头文件路径
# 假设你把 json.hpp 放在了 /usr/include/nlohmann/ 或者项目根目录的 include 文件夹
# 下面假设已通过 apt 安装（头文件在 /usr/include/nlohmann）
# 如果你的 json.hpp 在项目根目录的 include 下，可以取消注释下一行
# include_directories(include)

# 只搜索 src 目录下的 .cpp 文件（递归）
file(GLOB_RECURSE OTHER_SOURCES "src/*.cpp")

# 合并所有源文件
set(ALL_SOURCES ${OTHER_SOURCES} ${PROTO_SRCS})

# 生成可执行文件
add_executable(echo_server ${ALL_SOURCES})

# 包含生成的头文件目录（.pb.h 所在位置）
target_include_directories(echo_server PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)


# 链接线程库
target_link_libraries(echo_server
	Threads::Threads
	${Protobuf_LIBRARIES}
)

# 可选：如果需要调试，可以设置编译选项
target_compile_options(echo_server PRIVATE -g -O0)

# 运行 cmake 后（甚至在编译前）就在 build 目录下看到配置文件 config
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/
    DESTINATION ${CMAKE_BINARY_DIR}/config
)

# 复制 config 目录下的所有 .ini 文件到 build 根目录
#file(GLOB CONFIG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/*.ini)
#foreach(CONFIG_FILE ${CONFIG_FILES})
#    file(COPY ${CONFIG_FILE} DESTINATION ${CMAKE_BINARY_DIR})
#endforeach()