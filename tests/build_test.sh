#!/bin/bash

# PitchAnalyzer 단위 테스트 빌드 스크립트

echo "========================================="
echo "  PitchAnalyzer 테스트 빌드"
echo "========================================="
echo ""

# 컴파일할 소스 파일들
CPP_FILES=(
    "tests/test_pitch_analyzer.cpp"
    "src/audio/AudioBuffer.cpp"
    "src/analysis/PitchAnalyzer.cpp"
)

# 출력 파일명
OUTPUT="tests/test_pitch_analyzer"

echo "컴파일 중..."
echo ""

# 네이티브 C++ 컴파일 (Emscripten 없이)
g++ "${CPP_FILES[@]}" \
    -o "$OUTPUT" \
    -std=c++17 \
    -I. \
    -O2

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ 빌드 완료!"
    echo ""
    echo "생성된 파일: ./$OUTPUT"
    echo ""
    echo "실행 방법:"
    echo "  ./$OUTPUT original.wav"
    echo ""
    echo "결과 파일:"
    echo "  - pitch_analysis.csv (CSV 형식의 분석 결과)"
    echo ""
else
    echo ""
    echo "✗ 빌드 실패!"
    exit 1
fi
