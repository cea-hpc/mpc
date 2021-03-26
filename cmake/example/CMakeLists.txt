cmake_minimum_required(VERSION 3.0)

project(mpcmain)

set(MPCFRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

add_executable(main t.c)

find_package(MPCFRAMEWORK REQUIRED VERSION 4.0.0)

include_directories(${MPCFRAMEWORK_INCLUDE_DIRS})
target_link_libraries (main ${MPCFRAMEWORK_LIBRARIES})
