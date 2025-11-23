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
    # RubberBand 라이브러리 (Single compilation unit)
    "src/external/rubberband/single/RubberBandSingle.cpp"
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
  --bind \
  -O3 \
  -I./src \
  -I./src/external/soundtouch/include \
  -I./src/external/soundtouch/source \
  -I./src/external/kissfft \
  -I./src/external/rubberband

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
cp web/index.html dist/
cp -r web/css dist/
cp -r web/js dist/
cp -r web/components dist/

echo "✓ Static files copied!"
echo ""

# benchmark_result 복사 (심볼릭 링크 대신)
if [ -d "benchmark_result" ]; then
    echo "Copying benchmark results..."
    cp -r benchmark_result dist/benchmark
    echo "✓ Benchmark results copied!"
else
    echo "⚠ benchmark_result directory not found, skipping..."
fi

echo ""
echo "========================================"
echo "✓ Distribution build completed!"
echo "========================================"
echo ""
echo "Generated files in dist/:"
echo "  - dist/main.js"
echo "  - dist/main.wasm"
echo "  - dist/index.html"
echo "  - dist/css/"
echo "  - dist/js/"
echo "  - dist/components/"
echo "  - dist/benchmark/ (if available)"
echo ""
echo "Ready for deployment!"
echo ""
