#!/bin/bash

echo "====================================="
echo "Building All Benchmarks"
echo "====================================="
echo ""

# 공통 소스 파일들
COMMON_SOURCES=(
    "../src/audio/AudioBuffer.cpp"
    "../src/analysis/PitchAnalyzer.cpp"
    "../src/analysis/DurationAnalyzer.cpp"
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

# 1. Partial Segment Benchmark
echo "[1/4] Building Partial Segment Benchmark..."
PARTIAL_SOURCES=(
    "benchmark_partialsegment.cpp"
    "../src/effects/FastTimeStretchStrategy.cpp"
    "../src/effects/HighQualityTimeStretchStrategy.cpp"
    "../src/effects/ExternalTimeStretchStrategy.cpp"
    "../src/effects/PhaseVocoderTimeStretchStrategy.cpp"
    "../src/effects/RubberBandTimeStretchStrategy.cpp"
    "../src/effects/PhaseVocoder.cpp"
    "../src/benchmark/PartialSegmentBenchmark.cpp"
)

clang++ $COMMON_FLAGS "${PARTIAL_SOURCES[@]}" "${COMMON_SOURCES[@]}" -o benchmark_partialsegment

if [ $? -eq 0 ]; then
    echo "✓ Partial Segment Benchmark built successfully"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "✗ Partial Segment Benchmark build failed"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""

# 2. Pitch Shift Benchmark
echo "[2/4] Building Pitch Shift Benchmark..."
PITCH_SOURCES=(
    "benchmark_pitchshift.cpp"
    "../src/effects/PitchShifter.cpp"
    "../src/effects/FastPitchShiftStrategy.cpp"
    "../src/effects/HighQualityPitchShiftStrategy.cpp"
    "../src/effects/ExternalPitchShiftStrategy.cpp"
    "../src/effects/PSOLAPitchShiftStrategy.cpp"
    "../src/effects/RubberBandPitchShiftStrategy.cpp"
    "../src/effects/PhaseVocoder.cpp"
    "../src/effects/PhaseVocoderPitchShifter.cpp"
    "../src/benchmark/PitchShiftBenchmark.cpp"
)

clang++ $COMMON_FLAGS "${PITCH_SOURCES[@]}" "${COMMON_SOURCES[@]}" -o benchmark_pitchshift

if [ $? -eq 0 ]; then
    echo "✓ Pitch Shift Benchmark built successfully"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "✗ Pitch Shift Benchmark build failed"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""

# 2. Time Stretch Benchmark
echo "[3/4] Building Time Stretch Benchmark..."
TIME_SOURCES=(
    "benchmark_timestretch.cpp"
    "../src/effects/FastTimeStretchStrategy.cpp"
    "../src/effects/HighQualityTimeStretchStrategy.cpp"
    "../src/effects/ExternalTimeStretchStrategy.cpp"
    "../src/effects/PhaseVocoder.cpp"
    "../src/effects/PhaseVocoderTimeStretchStrategy.cpp"
    "../src/effects/RubberBandTimeStretchStrategy.cpp"
    "../src/benchmark/TimeStretchBenchmark.cpp"
)

clang++ $COMMON_FLAGS "${TIME_SOURCES[@]}" "${COMMON_SOURCES[@]}" -o benchmark_timestretch

if [ $? -eq 0 ]; then
    echo "✓ Time Stretch Benchmark built successfully"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "✗ Time Stretch Benchmark build failed"
    FAIL_COUNT=$((FAIL_COUNT + 1))
fi

echo ""

# 3. Combined Benchmark
echo "[4/4] Building Combined Benchmark..."
COMBINED_SOURCES=(
    "benchmark_combined.cpp"
    "../src/effects/ExternalPitchShiftStrategy.cpp"
    "../src/effects/ExternalTimeStretchStrategy.cpp"
    "../src/effects/HighQualityPitchShiftStrategy.cpp"
    "../src/effects/PhaseVocoder.cpp"
    "../src/effects/PhaseVocoderPitchShifter.cpp"
    "../src/effects/PhaseVocoderTimeStretchStrategy.cpp"
    "../src/effects/RubberBandTimeStretchStrategy.cpp"
    "../src/benchmark/CombinedBenchmark.cpp"
)

clang++ $COMMON_FLAGS "${COMBINED_SOURCES[@]}" "${COMMON_SOURCES[@]}" -o benchmark_combined

if [ $? -eq 0 ]; then
    echo "✓ Combined Benchmark built successfully"
    SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
    echo "✗ Combined Benchmark build failed"
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
    echo "  ./benchmark_partialsegment ../original.wav"
    echo "  ./benchmark_pitchshift ../original.wav"
    echo "  ./benchmark_timestretch ../original.wav 1.5"
    echo "  ./benchmark_combined ../original.wav 3.0 1.5"
    exit 0
else
    echo "✗ Some benchmarks failed to build"
    exit 1
fi
