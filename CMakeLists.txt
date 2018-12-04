cmake_minimum_required(VERSION 3.12)
project(rsyslog_awslib)

include_directories(
        /home/mschuett/projects/aws-sdk-cpp/aws-cpp-sdk-core/include
        /home/mschuett/projects/aws-sdk-cpp/aws-cpp-sdk-logs/include
)

set(CMAKE_CXX_STANDARD 11)
# set (CMAKE_CXX_FLAGS "-fPIC")
set (CMAKE_LIBRARY_PATH
        /home/mschuett/projects/aws-sdk-cpp/cmake-build-debug/.deps/install/lib
)
add_library(aws-cpp-sdk-core SHARED IMPORTED)
add_library(aws-cpp-sdk-logs SHARED IMPORTED)
add_library(aws-c-event-stream SHARED IMPORTED)
add_library(aws-c-common SHARED IMPORTED)
add_library(aws-checksums SHARED IMPORTED)
add_library(rsyslog_awslib SHARED awslib.cpp awslib.h)

set_target_properties(aws-cpp-sdk-core PROPERTIES IMPORTED_LOCATION /home/mschuett/projects/aws-sdk-cpp/cmake-build-debug/aws-cpp-sdk-core/libaws-cpp-sdk-core.so)
set_target_properties(aws-cpp-sdk-logs PROPERTIES IMPORTED_LOCATION /home/mschuett/projects/aws-sdk-cpp/cmake-build-debug/aws-cpp-sdk-logs/libaws-cpp-sdk-logs.so)
set_target_properties(aws-c-event-stream PROPERTIES IMPORTED_LOCATION /home/mschuett/projects/aws-sdk-cpp/cmake-build-debug/.deps/install/lib/libaws-c-event-stream.so)
set_target_properties(aws-c-common PROPERTIES IMPORTED_LOCATION /home/mschuett/projects/aws-sdk-cpp/cmake-build-debug/.deps/install/lib/libaws-c-common.so)
set_target_properties(aws-checksums PROPERTIES IMPORTED_LOCATION /home/mschuett/projects/aws-sdk-cpp/cmake-build-debug/.deps/install/lib/libaws-checksums.so)


TARGET_LINK_LIBRARIES(rsyslog_awslib aws-cpp-sdk-core aws-cpp-sdk-logs)

add_executable(test test.cpp)
TARGET_LINK_LIBRARIES(test rsyslog_awslib aws-checksums aws-c-common aws-c-event-stream aws-cpp-sdk-core aws-cpp-sdk-logs)
