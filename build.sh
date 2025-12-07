#!/bin/bash

EMSDK_DIR="./emsdk"

# Emscripten이 설치되어 있는지 확인
if [ ! -d "$EMSDK_DIR" ]; then
    echo "Emscripten이 설치되어 있지 않습니다."
    echo "setup_emscripten.sh를 먼저 실행해주세요."
    echo ""
    echo "  ./setup_emscripten.sh"
    echo ""
    exit 1
fi

# Emscripten 환경 활성화
echo "Emscripten 환경을 활성화합니다..."
source ./emsdk/emsdk_env.sh

# 모든 C++ 소스 파일 수집
echo "빌드를 시작합니다..."
echo ""

CPP_FILES=(
    "src/main.cpp"
    "src/audio/AudioBuffer.cpp"
    "src/audio/AudioPreprocessor.cpp"
    "src/analysis/PitchAnalyzer.cpp"
    "src/effects/VoiceFilter.cpp"
    "src/effects/AudioReverser.cpp"
    "src/performance/PerformanceChecker.cpp"
    # 직접 구현한 DSP 알고리즘
    "src/dsp/SimplePitchShifter.cpp"
    "src/dsp/SimpleTimeStretcher.cpp"
    # SoundTouch 라이브러리 (핵심 파일만)
    "src/external/soundtouch/source/SoundTouch/SoundTouch.cpp"
    "src/external/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp"
    "src/external/soundtouch/source/SoundTouch/RateTransposer.cpp"
    "src/external/soundtouch/source/SoundTouch/TDStretch.cpp"
    "src/external/soundtouch/source/SoundTouch/AAFilter.cpp"
    "src/external/soundtouch/source/SoundTouch/FIRFilter.cpp"
    "src/external/soundtouch/source/SoundTouch/InterpolateLinear.cpp"
    "src/external/soundtouch/source/SoundTouch/InterpolateCubic.cpp"
    "src/external/soundtouch/source/SoundTouch/InterpolateShannon.cpp"
    "src/external/soundtouch/source/SoundTouch/PeakFinder.cpp"
    "src/external/soundtouch/source/SoundTouch/cpu_detect_x86.cpp"
    # KissFFT 라이브러리
    "src/external/kissfft/kiss_fft.c"
)

# 컴파일
em++ "${CPP_FILES[@]}" \
  -o web/cpp/main.js \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='["_malloc", "_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPF32"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='Module' \
  -s EXPORT_ES6=0 \
  -s ENVIRONMENT='web' \
  -s SINGLE_FILE=0 \
  --bind \
  -O3 \
  -I./src \
  -I./src/external/soundtouch/include \
  -I./src/external/soundtouch/source \
  -I./src/external/kissfft

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ 빌드 완료!"
    echo ""
    echo "생성된 파일:"
    echo "  - web/cpp/main.js"
    echo "  - web/cpp/main.wasm"
    echo ""
    echo "웹 서버 실행:"
    echo "  ./runserver.sh"
    echo ""
else
    echo ""
    echo "✗ 빌드 실패!"
    exit 1
fi
