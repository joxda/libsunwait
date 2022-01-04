cmake_minimum_required(VERSION 3.16)

project(sunwait)


add_library(sunwait libsunwait.cpp  sun.cpp sunarc.cpp ) 
add_library(sunwait::sunwait ALIAS sunwait)

set_property(TARGET sunwait PROPERTY CXX_STANDARD 11 )
set_property(TARGET sunwait PROPERTY PUBLIC_HEADER libsunwait.hpp)
set_property(TARGET sunwait PROPERTY VERSION 1.0.2)
add_compile_options( -Wall -Wextra -Wpedantic -Werror )

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
add_executable(test test.cpp )
target_link_libraries(test PRIVATE sunwait)
set_property(TARGET test PROPERTY CXX_STANDARD 11 )
endif()

install(TARGETS sunwait 
	EXPORT sunwaitTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
export(EXPORT sunwaitTargets
  NAMESPACE sunwait::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/sunwait.cmake"
)
set(CPACK_GENERATOR "TGZ")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Joachim Janz") # required
set(CPACK_PACKAGE_VENDOR "joxda")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)
set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

include(CPack)
