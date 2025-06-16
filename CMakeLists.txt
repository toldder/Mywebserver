#设置版本号和项目名称
cmake_minimum_required(VERSION 3.15)
project(webserver)

#设置c++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

#添加头文件
include_directories(${PROJECT_SOURCE_DIR}/include)

#设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

#添加子目录
add_subdirectory(memory)
add_subdirectory(log)
add_subdirectory(src)
add_subdirectory(http)

#设置全局链接库
set(LIBS pthread)