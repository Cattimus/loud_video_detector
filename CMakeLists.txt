cmake_minimum_required(VERSION 3.15)
project(loud_detector)

add_compile_options(-O3 -s)
add_executable(loud_detector src/main.cpp src/wav_16.cpp src/audio_processing.cpp)
