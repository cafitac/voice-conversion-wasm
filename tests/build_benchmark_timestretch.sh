#!/bin/bash

echo "Building Time Stretch Benchmark..."

# 소스 파일들
SOURCES=(
    "benchmark_timestretch.cpp"
    "../src/audio/AudioBuffer.cpp"
    "../src/analysis/PitchAnalyzer.cpp"
    "../src/analysis/DurationAnalyzer.cpp"
    "../src/effects/FastTimeStretchStrategy.cpp"
    "../src/effects/HighQualityTimeStretchStrategy.cpp"
    "../src/effects/ExternalTimeStretchStrategy.cpp"
    "../src/utils/WaveFile.cpp"
    "../src/utils/FFTWrapper.cpp"
    # SoundTouch 라이브러리
    "../src/external/soundtouch/source/SoundTouch/SoundTouch.cpp"
    "../src/external/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp"
    "../src/external/soundtouch/source/SoundTouch/RateTransposer.cpp"
    "../src/external/soundtouch/source/SoundTouch/TDStretch.cpp"
    "../src/external/soundtouch/source/SoundTouch/InterpolateLinear.cpp"
    "../src/external/soundtouch/source/SoundTouch/InterpolateCubic.cpp"
    "../src/external/soundtouch/source/SoundTouch/InterpolateShannon.cpp"
    "../src/external/soundtouch/source/SoundTouch/AAFilter.cpp"
    "../src/external/soundtouch/source/SoundTouch/FIRFilter.cpp"
    "../src/external/soundtouch/source/SoundTouch/BPMDetect.cpp"
    "../src/external/soundtouch/source/SoundTouch/PeakFinder.cpp"
    "../src/external/soundtouch/source/SoundTouch/cpu_detect_x86.cpp"
    "../src/external/soundtouch/source/SoundTouch/sse_optimized.cpp"
)

# 컴파일
clang++ -std=c++17 -O3 \
    -I../src/external/soundtouch/include \
    -I../src/external/kissfft \
    "${SOURCES[@]}" \
    ../src/external/kissfft/kiss_fft.c \
    -o benchmark_timestretch \
    -lm

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo "Run: ./benchmark_timestretch [input.wav] [ratio]"
    echo "Example: ./benchmark_timestretch ../original.wav 1.5"
else
    echo "✗ Build failed!"
    exit 1
fi
