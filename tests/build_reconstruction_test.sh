#!/bin/bash

echo "재구성 테스트 프로그램 빌드 중..."
echo ""

CPP_FILES=(
    "tests/test_reconstruction.cpp"
    "src/audio/AudioBuffer.cpp"
    "src/audio/AudioPreprocessor.cpp"
    "src/effects/PitchShifter.cpp"
    "src/effects/TimeStretcher.cpp"
    "src/effects/FramePitchModifier.cpp"
    "src/effects/TimeScaleModifier.cpp"
    "src/effects/PhaseVocoder.cpp"
    "src/effects/PhaseVocoderPitchShifter.cpp"
    "src/synthesis/FrameReconstructor.cpp"
    "src/utils/FFTWrapper.cpp"
    "src/utils/WaveFile.cpp"
    "src/external/kissfft/kiss_fft.c"
)

# 네이티브 컴파일 (macOS/Linux)
g++ "${CPP_FILES[@]}" \
  -o tests/test_reconstruction \
  -std=c++11 \
  -I. \
  -O2

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ 빌드 완료!"
    echo ""
    echo "실행:"
    echo "  ./tests/test_reconstruction"
    echo ""
else
    echo ""
    echo "✗ 빌드 실패!"
    exit 1
fi
