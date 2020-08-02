
cmake_minimum_required(VERSION 3.4.1)

file(GLOB my_source_path ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h SafeQueue.h SafeQueue.cpp)


include_directories(${CMAKE_SOURCE_DIR}/ffmpeg/include)
include_directories(${CMAKE_SOURCE_DIR}/fdkaac/include)
include_directories(${CMAKE_SOURCE_DIR}/rtmp/include)
include_directories(${CMAKE_SOURCE_DIR}/x264/include)
include_directories(${CMAKE_SOURCE_DIR}/libyuv/include)

LINK_DIRECTORIES("${CMAKE_SOURCE_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/")

add_library(
        native-lib
        SHARED
        ${my_source_path}
)


find_library(
        log-lib
        log
)

target_link_libraries(
        native-lib
        ${log-lib}
        -Wl,--start-group
        avcodec avfilter avformat avutil swresample swscale postproc avdevice
        -Wl,--end-group
        z
        rtmp
        x2641
        fdk-aac
        yuv
        EGL # EGL设置Surface
        GLESv3 #OpenGL渲染屏幕
        android #渲染屏幕
)