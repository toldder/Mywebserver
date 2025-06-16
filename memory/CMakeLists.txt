#设置版本号和项目名称
cmake_minimum_required(VERSION 3.15)
project(memory)

#添加头文件
include_directories(${PROJECT_SOURCE_DIR}/include)

#获取当前目录下的所有源文件项目
file(GLOB MEMORY_FILE ${PROJECT_SOURCE_DIR}/*.cc)

#创建动态库
add_library(memory_lib SHARED ${MEMORY_FILE})

