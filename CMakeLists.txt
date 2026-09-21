cmake_minimum_required(VERSION 3.20)
project(Lufia2Decomp VERSION 0.1.0 LANGUAGES C)

add_library(lufia2_decomp STATIC
    src/player_update.c)
add_library(Lufia2::Decomp ALIAS lufia2_decomp)

target_include_directories(lufia2_decomp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(lufia2_decomp PUBLIC c_std_11)
