#!/bin/bash

echo "========================================"
echo "Building for distribution (dist/)"
echo "========================================"
echo ""

# Emscripten 환경 활성화 (로컬 빌드용, CI는 자동)
if [ -d "./emsdk" ]; then
    echo "Activating local Emscripten environment..."
    source ./emsdk/emsdk_env.sh
fi

# 모든 C++ 소스 파일 수집
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

# dist 디렉토리 생성
echo "Creating dist/ directory..."
mkdir -p dist

# WASM 빌드 (dist/로 출력)
echo "Building WASM..."
em++ "${CPP_FILES[@]}" \
  -o dist/main.js \
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

if [ $? -ne 0 ]; then
    echo ""
    echo "✗ WASM build failed!"
    exit 1
fi

echo ""
echo "✓ WASM build completed!"
echo ""

# web/ 정적 파일 복사
echo "Copying static files from web/..."
mkdir -p dist/cpp dist/js dist/app

# 메인 라우팅 페이지
cp web/index.html dist/

# 통합 앱 (NEW)
cp web/app/index.html dist/app/
cp -r web/app/css dist/app/
cp -r web/app/js dist/app/
cp -r web/app/components dist/app/ 2>/dev/null || true

# C++ 버전
cp web/cpp/index.html dist/cpp/
cp web/cpp/editor.html dist/cpp/ 2>/dev/null || true
cp -r web/cpp/css dist/cpp/
cp -r web/cpp/js dist/cpp/
cp -r web/cpp/components dist/cpp/ 2>/dev/null || true

# JavaScript 버전
cp web/js/index.html dist/js/
cp -r web/js/css dist/js/
cp -r web/js/js dist/js/

# WASM 파일을 C++ 버전과 통합 앱으로 복사
cp dist/main.js dist/cpp/
cp dist/main.wasm dist/cpp/
cp dist/main.js dist/app/
cp dist/main.wasm dist/app/

echo "✓ Static files copied!"
echo ""

echo "========================================"
echo "✓ Distribution build completed!"
echo "========================================"
echo ""
echo "Generated files in dist/:"
echo "  - dist/index.html (메인 라우팅)"
echo "  - dist/app/ (통합 앱 - C++/JS 토글)"
echo "  - dist/cpp/ (C++ WASM 버전)"
echo "  - dist/js/ (JavaScript 버전)"
echo ""
echo "Ready for deployment!"
echo ""
