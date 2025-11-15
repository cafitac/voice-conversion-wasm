#!/bin/bash

echo "=== C++ 편집 파이프라인 테스트 빌드 ==="
echo ""

# 필수 소스 파일들
g++ -std=c++17 -o test_edit_pipeline \
    test_edit_pipeline.cpp \
    ../src/utils/EditPointGenerator.cpp \
    ../src/analysis/PitchAnalyzer.cpp \
    ../src/audio/AudioBuffer.cpp \
    ../src/utils/FFTWrapper.cpp \
    ../src/external/kissfft/kiss_fft.c \
    -I../src \
    -I../src/external/kissfft \
    -lm

if [ $? -eq 0 ]; then
    echo "✓ 빌드 성공!"
    echo ""
    echo "실행:"
    echo "  ./test_edit_pipeline"
    echo ""
else
    echo "✗ 빌드 실패!"
    exit 1
fi
