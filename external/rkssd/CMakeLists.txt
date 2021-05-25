# cmake needs this line
cmake_minimum_required(VERSION 2.8)

# Define project name
project(rkssddemo)

add_definitions(-std=c++11)

# set cross compiler path
set(CMAKE_C_COMPILER   /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")

# import rkssd
include_directories(dep/rkssd/include)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/dep/rkssd/lib)

# import opencv
include_directories(dep/opencv/include)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/dep/opencv/lib)

set(CMAKE_INSTALL_RPATH "lib")

# Declare the executable target built from your sources
add_executable(rkssddemo src/main.cpp)

# Link libraries
target_link_libraries(rkssddemo librkssd.so)
target_link_libraries(rkssddemo libopencv_core.so)
target_link_libraries(rkssddemo libopencv_highgui.so)
target_link_libraries(rkssddemo libopencv_imgcodecs.so)
target_link_libraries(rkssddemo libopencv_imgproc.so)

# install target and libraries
install(TARGETS rkssddemo DESTINATION rkssddemo)
set(DEPENDENCIES
        ${CMAKE_SOURCE_DIR}/dep/rkssd/lib/librkssd.so
        ${CMAKE_SOURCE_DIR}/dep/opencv/lib/libopencv_core.so.3.4
        ${CMAKE_SOURCE_DIR}/dep/opencv/lib/libopencv_highgui.so.3.4
        ${CMAKE_SOURCE_DIR}/dep/opencv/lib/libopencv_imgcodecs.so.3.4
        ${CMAKE_SOURCE_DIR}/dep/opencv/lib/libopencv_imgproc.so.3.4
        )
foreach(DEPENDENCY ${DEPENDENCIES})
    install(PROGRAMS "${DEPENDENCY}" DESTINATION rkssddemo/lib)
endforeach()
