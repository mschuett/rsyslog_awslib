cmake_minimum_required(VERSION 3.0)
project(librsyslog_aws VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
include(GNUInstallDirs)

add_library(rsyslog_awslogs SHARED rsyslog_awslogs.cpp rsyslog_awslogs.h)

find_package(AWSSDK REQUIRED COMPONENTS logs)
target_link_libraries(rsyslog_awslogs ${AWSSDK_LINK_LIBRARIES})

install (TARGETS rsyslog_awslogs DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES rsyslog_awslogs.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# packaging
SET(CPACK_GENERATOR "DEB" "RPM")
SET(CPACK_PACKAGE_VENDOR "Martin Schuette")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AWS library for rsyslog, used for omawslogs")
SET(CPACK_PACKAGE_CONTACT "Martin Schuette <info@mschuette.name>")
SET(CPACK_PACKAGE_HOMEPAGE "https://github.com/mschuett/rsyslog_awslib")
SET(CPACK_PACKAGE_DEPENDS "libaws-sdk-cpp, rsyslog")
SET(CPACK_RPM_PACKAGE_DESCRIPTION "Support library, used for the AWS Rsyslog output plugin.")
SET(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
#SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
INCLUDE(CPack)
