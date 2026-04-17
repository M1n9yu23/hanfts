cmake_minimum_required(VERSION 3.22.1)
project("hanfts" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(
    hanfts
    SHARED
    src/tokenizer.cpp
    src/inverted_index.cpp
    src/search_engine.cpp
    src/fts_jni.cpp
)

target_include_directories(hanfts PRIVATE include)

target_compile_options(hanfts PRIVATE
    -O2
    -fvisibility=hidden
    -Wall
)

target_link_options(hanfts PRIVATE "-Wl,-z,max-page-size=16384")
