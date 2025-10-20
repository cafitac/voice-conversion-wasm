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
    "src/audio/AudioRecorder.cpp"
    "src/audio/AudioProcessor.cpp"
    "src/analysis/PitchAnalyzer.cpp"
    "src/analysis/DurationAnalyzer.cpp"
    "src/effects/PitchShifter.cpp"
    "src/effects/TimeStretcher.cpp"
    "src/effects/VoiceFilter.cpp"
    "src/utils/WaveFile.cpp"
    "src/visualization/CanvasRenderer.cpp"
)

# 컴파일
em++ "${CPP_FILES[@]}" \
  -o web/main.js \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='["_malloc", "_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPF32"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='Module' \
  -s EXPORT_ES6=0 \
  --bind \
  -O3 \
  -I./src

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ 빌드 완료!"
    echo ""
    echo "생성된 파일:"
    echo "  - web/main.js"
    echo "  - web/main.wasm"
    echo ""
    echo "웹 서버 실행:"
    echo "  ./runserver.sh"
    echo ""
else
    echo ""
    echo "✗ 빌드 실패!"
    exit 1
fi
