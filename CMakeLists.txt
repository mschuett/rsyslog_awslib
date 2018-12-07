cmake_minimum_required(VERSION 3.0)
project(librsyslog_aws VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_library(rsyslog_awslib SHARED awslib.cpp awslib.h)

find_package(AWSSDK REQUIRED COMPONENTS logs)
target_link_libraries(rsyslog_awslib ${AWSSDK_LINK_LIBRARIES})
