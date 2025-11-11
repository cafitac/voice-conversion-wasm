cmake_minimum_required(VERSION 3.10)
project(VoiceManipulation)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일 디렉토리
include_directories(${CMAKE_SOURCE_DIR})

# 테스트 서브디렉토리 추가
add_subdirectory(tests)
