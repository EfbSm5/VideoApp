cmake_minimum_required(VERSION 3.10.2)

project("videoapp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

# 路径：预编译 FFmpeg so 和头文件
set(ffmpeg_lib_dir ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})
set(ffmpeg_head_dir ${CMAKE_SOURCE_DIR}/include)
#set(fftool_c_dir ${CMAKE_SOURCE_DIR}/fftools)

include_directories(${ffmpeg_head_dir}
#        ${fftool_c_dir}
#        ${ffmpeg_head_dir}/fftools
)

# 导入预编译 FFmpeg so
add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavcodec.so)


add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libavutil.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${ffmpeg_lib_dir}/libswresample.so)


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
#        ${fftool_c_dir}/cmdutils.c
#        ${fftool_c_dir}/ffmpeg.c
#        ${fftool_c_dir}/ffmpeg_demux.c
#        ${fftool_c_dir}/ffmpeg_mux.c
#        ${fftool_c_dir}/ffmpeg_dec.c
#        ${fftool_c_dir}/ffmpeg_enc.c
#        ${fftool_c_dir}/ffmpeg_filter.c
#        ${fftool_c_dir}/ffmpeg_hw.c
#        ${fftool_c_dir}/opt_common.c
#        ${fftool_c_dir}/sync_queue.c
#        ${fftool_c_dir}/thread_queue.c
#        ${fftool_c_dir}/objpool.c
)
#
#set_source_files_properties(
#        ${fftool_c_dir}/ffmpeg.c
#        PROPERTIES COMPILE_FLAGS "-Dmain=ffmpeg_main"
#)

#add_library(ffmpeg
#        SHARED
#        fftools/cmdutils.c
#        fftools/ffmpeg_entry.c
#        fftools/ffmpeg_filter.c
#        fftools/ffmpeg_hw.c
#        fftools/ffmpeg_opt.c
#)

# 链接
target_link_libraries(
        native-lib
        avutil
        swresample
        avcodec
        avfilter
        swscale
        avformat
        avdevice
        ${log-lib}
)
#target_link_libraries(
#        native-lib
#        ffmpeg
#        ${log-lib})




