cmake_minimum_required(VERSION 3.10.2)

project("videoapp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

# 路径：预编译 FFmpeg so 和头文件
set(ffmpeg_lib_dir ${CMAKE_SOURCE_DIR}/../jnilibs/${ANDROID_ABI})
set(ffmpeg_head_dir ${CMAKE_SOURCE_DIR}/include)

# 头文件目录（例如 libavformat/avformat.h 等）
include_directories(${ffmpeg_head_dir})

# 导入预编译 FFmpeg so
add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavutil.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libswresample.so)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavcodec.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavfilter.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libswscale.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavformat.so)

add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavdevice.so)

# NDK 日志库
find_library(log-lib log)

# 我们自己的 so
add_library(native-lib SHARED
        native-lib.cpp
)

# 链接
target_link_libraries(native-lib
        avutil
        swresample
        avcodec
        avfilter
        swscale
        avformat
        avdevice
        ${log-lib}
)
