cmake_minimum_required(VERSION 3.16.3)

project(media-processer)
set(VIDEO_PROCESSER_LIB_NAME videoprocesser)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/util MEDIA_RECORDER_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src MEDIA_RECORDER_SRC)

add_library(${VIDEO_PROCESSER_LIB_NAME} SHARED ${MEDIA_RECORDER_SRC})
add_executable(demo-recorder ${CMAKE_CURRENT_SOURCE_DIR}/demo_recorder.cpp)
target_link_libraries(demo-recorder ${VIDEO_PROCESSER_LIB_NAME} avformat avcodec avutil yuv x264)

add_executable(demo-split ${CMAKE_CURRENT_SOURCE_DIR}/demo_split.cpp)
target_link_libraries(demo-split ${VIDEO_PROCESSER_LIB_NAME} avformat avcodec avutil yuv x264)
