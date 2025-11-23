#!/bin/bash

echo "====================================="
echo "Building All Benchmarks (Algorithm-based)"
echo "====================================="
echo ""

# 공통 소스 파일들 (Algorithm 기반으로 간소화)
COMMON_SOURCES=(
    "../src/audio/AudioBuffer.cpp"
    "../src/analysis/PitchAnalyzer.cpp"
    "../src/utils/WaveFile.cpp"
    "../src/utils/FFTWrapper.cpp"
    # Preprocessors
    "../src/preprocessor/AudioNoisePreprocessor.cpp"
    # Pitch Algorithms
    "../src/algorithm/pitch/PSOLAAlgorithm.cpp"
    "../src/algorithm/pitch/PhaseVocoderAlgorithm.cpp"
    "../src/algorithm/pitch/SoundTouchAlgorithm.cpp"
    "../src/algorithm/pitch/RubberBandAlgorithm.cpp"
    # Duration Algorithms
    "../src/algorithm/duration/WSOLAAlgorithm.cpp"
    "../src/algorithm/duration/SoundTouchDurationAlgorithm.cpp"
    "../src/algorithm/duration/RubberBandDurationAlgorithm.cpp"
    # Effects (for PhaseVocoder core)
    "../src/effects/PhaseVocoder.cpp"
    "../src/effects/PhaseVocoderPitchShifter.cpp"
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
    # KissFFT 라이브러리
    "../src/external/kissfft/kiss_fft.c"
    # RubberBand 라이브러리
    "../src/external/rubberband/single/RubberBandSingle.cpp"
)

# 공통 컴파일 플래그
COMMON_FLAGS="-std=c++17 -O3 \
    -I../src/external/soundtouch/include \
    -I../src/external/kissfft \
    -I../src/external/rubberband \
    -lm \
    -framework Accelerate"

SUCCESS_COUNT=0
FAIL_COUNT=0

# 1. Pitch Shift Benchmark (Processor-based)
echo "[1/2] Building Pitch Shift Benchmark (Algorithm-based)..."
PITCH_SOURCES=(
    "benchmark_pitchshift.cpp"
    "../src/benchmark/PitchShiftBenchmark.cpp"
)

clang++ $COMMON_FLAGS "${PITCH_SOURCES[@]}" "${COMMON_SOURCES[@]}" -o benchmark_pitchshift 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Pitch Shift Benchmark built successfully"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "✗ Pitch Shift Benchmark build failed"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""

# 2. Time Stretch Benchmark (Processor-based)
echo "[2/2] Building Time Stretch Benchmark (Algorithm-based)..."
TIME_SOURCES=(
    "benchmark_timestretch.cpp"
    "../src/benchmark/TimeStretchBenchmark.cpp"
)

clang++ $COMMON_FLAGS "${TIME_SOURCES[@]}" "${COMMON_SOURCES[@]}" -o benchmark_timestretch 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Time Stretch Benchmark built successfully"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "✗ Time Stretch Benchmark build failed"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""
echo "====================================="
echo "Build Summary"
echo "====================================="
echo "Success: $SUCCESS_COUNT"
echo "Failed: $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo "✓ All benchmarks built successfully!"
    echo ""
    echo "Run benchmarks:"
    echo "  ./benchmark_pitchshift ../original.wav"
    echo "  ./benchmark_timestretch ../original.wav"
    exit 0
else
    echo "✗ Some benchmarks failed to build"
    exit 1
fi
