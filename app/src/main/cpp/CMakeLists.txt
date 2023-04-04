cmake_minimum_required(VERSION 3.22.1)
project("supercube")

set(ANDROID_STL "none")
enable_language(ASM)
set(CMAKE_ASM_SOURCE_FILE_EXTENSIONS s)

add_library(matrix STATIC matrix.cpp matrix_mul_neo.s)

add_library(native_gles3 STATIC native_gles3.cpp)
add_library(native_gles2 STATIC native_gles2.cpp)
add_library(native_gles STATIC native_gles.cpp)

add_library(native_sles STATIC native_sles.cpp)

add_library(super_cube1 SHARED native_activity_impl.cpp)
add_library(super_cube2 SHARED native_activity_impl.cpp)
add_library(super_cube3 SHARED native_activity_impl.cpp)

add_library(native_app_glue STATIC android_app_glue/android_native_app_glue.c)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(native_gles3
        log GLESv3 matrix)
target_link_libraries(native_gles2
        log GLESv2 matrix)
target_link_libraries(native_gles
        log GLESv1_CM matrix)

target_link_libraries(native_sles
        log OpenSLES)

target_link_libraries(super_cube1
        log android EGL GLESv3 native_app_glue native_sles native_gles)
target_link_libraries(super_cube2
        log android EGL GLESv3 native_app_glue native_sles native_gles2)
target_link_libraries(super_cube3
        log android EGL GLESv3 native_app_glue native_sles native_gles3)
