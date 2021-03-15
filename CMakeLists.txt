cmake_minimum_required(VERSION 3.17)

project(sunwait)


add_library(sunwait  libsunwait.cpp  sun.cpp sunarc.cpp ) 
set_property(TARGET sunwait PROPERTY CXX_STANDARD 11 )
set_property(TARGET sunwait PROPERTY PUBLIC_HEADER libsunwait.hpp)

add_executable(test test.cpp )
target_link_libraries(test PRIVATE sunwait)
set_property(TARGET test PROPERTY CXX_STANDARD 11 )


install(TARGETS sunwait DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS sunwait PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(CPACK_GENERATOR "TGZ")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Joachim Janz") # required
set(CPACK_PACKAGE_VENDOR "joxda")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

include(CPack)