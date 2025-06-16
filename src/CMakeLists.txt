#设置版本号和项目名称
cmake_minimum_required(VERSION 3.15)
project(src)

#获取当前目录下的源文件
file(GLOB SRC_FILE ${PROJECT_SOURCE_DIR}/*.cc)
list(REMOVE_ITEM SRC_FILE ${PROJECT_SOURCE_DIR}/echoServer.cc)

#创建动态库
add_library(src_lib SHARED ${SRC_FILE})

#生成可执行文件
add_executable(echoServer echoServer.cc)

link_directories(${PROJECT_SOURCE_DIR}/lib)
#链接静态库
target_link_libraries(echoServer src_lib memory_lib log_lib ${LIBS})