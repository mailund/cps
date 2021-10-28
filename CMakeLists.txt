cmake_minimum_required(VERSION 3.10)

project(cps)
set(CMAKE_C_STANDARD 11)
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/Modules/
)

include(CTest)
include(GroupFiles)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(src) 
add_subdirectory(examples)

