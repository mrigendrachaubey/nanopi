# vi: set noexpandtab syntax=cmake:

PROJECT(rk_image_process)
INCLUDE(GNUInstallDirs)

find_package(gflags REQUIRED)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil>=55.78.100)
pkg_check_modules(LIBDRM REQUIRED libdrm>=2.4.89)

set (CMAKE_CXX_STANDARD 11)

# add_definitions(-fexceptions -finstrument-functions -funwind-tables -g -rdynamic -O0)
add_definitions(-O2)

if (DEFINED DRAW_BY_OPENGLES)
	add_definitions(-DDRAW_BY_OPENGLES=${DRAW_BY_OPENGLES})
endif()

if (DEFINED DRAW_BY_SDL)
	add_definitions(-DDRAW_BY_SDL=${DRAW_BY_SDL})
endif()

if (DEFINED DRAW_BY_DRM)
	add_definitions(-DDRAW_BY_DRM=${DRAW_BY_DRM})
endif()

if (DEFINED SDL_GLES2FUNCS_H_DIR)
    add_definitions(-I${SDL_GLES2FUNCS_H_DIR})
endif()

set(RK_IMAGE_PROCESS_LINK_LIBRARIES
    drm gflags avutil avformat avcodec rga SDL2 SDL2_ttf SDL2_gfx v4l2)
option(RKNNCASCADE "compile RknnCascade" OFF)
if(RKNNCASCADE)
    add_definitions(-DRKNNCASCADE=1)
    set(RK_IMAGE_PROCESS_LINK_LIBRARIES ${RK_IMAGE_PROCESS_LINK_LIBRARIES}
                                        rknncascade)
endif()

option(ROCKX "using rockx" OFF)
if (ROCKX)
include_directories(${ROCKX_HERDER_DIR})
add_definitions(-DHAVE_ROCKX=1)
set(RK_IMAGE_PROCESS_LINK_LIBRARIES ${RK_IMAGE_PROCESS_LINK_LIBRARIES}
                                    rockx rknn_api)
endif()

set(RK_IMAGE_PROCESS_SRC image_process.cc)

add_executable(rk_image_process ${RK_IMAGE_PROCESS_SRC})

target_include_directories(rk_image_process
                           PUBLIC ${GFLAGS_INCLUDE_DIRS}
                           PUBLIC ${LIBAVUTIL_INCLUDE_DIRS}
                           PUBLIC ${LIBDRM_INCLUDE_DIRS}
                          )

# add_definitions(-fsanitize=address -static-libasan -fno-stack-protector -fno-omit-frame-pointer -fno-var-tracking -g1)
# target_link_libraries(rk_image_process asan)
target_link_libraries(rk_image_process ${RK_IMAGE_PROCESS_LINK_LIBRARIES})

install(TARGETS rk_image_process RUNTIME DESTINATION "bin")
install(DIRECTORY ./ DESTINATION "bin"
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.sh"
        PATTERN "rockx" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        )

install(DIRECTORY resource DESTINATION "local/share/app_demo"
        FILES_MATCHING PATTERN "*.bmp" PATTERN "*.png")
if (ROCKX)
install(DIRECTORY resource/ DESTINATION "share/applications"
        FILES_MATCHING PATTERN "*.desktop")
endif()
