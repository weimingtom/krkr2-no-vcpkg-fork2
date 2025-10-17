#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.6)
project(json)
add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PUBLIC
    jsonPlugin.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin)