cmake_minimum_required(VERSION 3.16.3)

project(media_recoder)
set(MP4_VIDEO_RECORDER_LIB_NAME mp4videorecorder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/util MEDIA_RECORDER_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src MEDIA_RECORDER_SRC)

add_library(${MP4_VIDEO_RECORDER_LIB_NAME} SHARED ${MEDIA_RECORDER_SRC})
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/demo.cpp)
target_link_libraries(${PROJECT_NAME} ${MP4_VIDEO_RECORDER_LIB_NAME} avformat avcodec avutil yuv x264)
