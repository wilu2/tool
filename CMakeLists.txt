cmake_minimum_required(VERSION 3.0)

project(contract_compare CXX)

option(BUILD_DEMO "build demo" OFF)
option(BUILD_LUA "BUILD LUA" OFF)
option(LOG_DEBUG "log debug" OFF)
option(USE_SPDLOG "use spdlog" OFF)
option(CV_DEBUG "CV DEBUG" OFF)


if(LOG_DEBUG)
add_definitions(-DLOG_DEBUG)
endif()

if(USE_SPDLOG)
add_definitions(-DUSE_SPDLOG)
endif()

if(BUILD_LUA)
add_definitions(-DENABLE_LUA)
endif()

if(CV_DEBUG)
add_definitions(-DCV_DEBUG)
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
endif()
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

#add_definitions(-std=c++11)


include_directories(src)
include_directories(third_party)
include_directories(third_party/spdlog/include)
aux_source_directory(src SRC)
aux_source_directory(src/zss SRC)


add_library(contract_compare_static STATIC ${SRC} ${FORMAT_SRC})
add_definitions(-fPIC)
add_library(contract_compare SHARED ${SRC} ${FORMAT_SRC})
set_target_properties(contract_compare_static PROPERTIES OUTPUT_NAME "contract_compare")



target_link_libraries(contract_compare pthread)
if(BUILD_LUA)
target_link_libraries(contract_compare lua)
endif()

install(FILES  ${PROJECT_SOURCE_DIR}/src/contract_compare.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(FILES  ${PROJECT_SOURCE_DIR}/src/contract_compare_c.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS contract_compare DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)


if(BUILD_DEMO)
add_subdirectory(demo)
endif()
