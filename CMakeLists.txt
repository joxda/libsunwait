cmake_minimum_required(VERSION 3.17)

project(sunwait)


add_library(sunwait  libsunwait.cpp  sun.cpp sunarc.cpp ) 
set_property(TARGET sunwait PROPERTY CXX_STANDARD 11 )


add_executable(test test.cpp )
target_link_libraries(test PRIVATE sunwait)
set_property(TARGET test PROPERTY CXX_STANDARD 11 )
