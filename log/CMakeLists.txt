#设置版本号和项目名称
cmake_minimum_required(VERSION 3.15)
project(log)

#获取当前目录下的所有源文件项目
file(GLOB LOG_FILE ${PROJECT_SOURCE_DIR}/*.cc)

#创建动态库
add_library(log_lib SHARED ${LOG_FILE})