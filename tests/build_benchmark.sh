#!/bin/bash

echo "Building Pitch Shift Benchmark..."

# 소스 파일 목록
CPP_FILES=(
    "benchmark_pitchshift.cpp"
    "../src/audio/AudioBuffer.cpp"
    "../src/audio/AudioProcessor.cpp"
    "../src/analysis/PitchAnalyzer.cpp"
    "../src/effects/PitchShifter.cpp"
    "../src/effects/PhaseVocoder.cpp"
    "../src/effects/PhaseVocoderPitchShifter.cpp"
    "../src/effects/FastPitchShiftStrategy.cpp"
    "../src/effects/HighQualityPitchShiftStrategy.cpp"
    "../src/effects/ExternalPitchShiftStrategy.cpp"
    "../src/utils/WaveFile.cpp"
    "../src/utils/FFTWrapper.cpp"
    # SoundTouch 라이브러리
    "../src/external/soundtouch/source/SoundTouch/SoundTouch.cpp"
    "../src/external/soundtouch/source/SoundTouch/FIFOSampleBuffer.cpp"
    "../src/external/soundtouch/source/SoundTouch/RateTransposer.cpp"
    "../src/external/soundtouch/source/SoundTouch/TDStretch.cpp"
    "../src/external/soundtouch/source/SoundTouch/AAFilter.cpp"
    "../src/external/soundtouch/source/SoundTouch/FIRFilter.cpp"
    "../src/external/soundtouch/source/SoundTouch/InterpolateLinear.cpp"
    "../src/external/soundtouch/source/SoundTouch/InterpolateCubic.cpp"
    "../src/external/soundtouch/source/SoundTouch/InterpolateShannon.cpp"
    "../src/external/soundtouch/source/SoundTouch/PeakFinder.cpp"
    "../src/external/soundtouch/source/SoundTouch/cpu_detect_x86.cpp"
    # KissFFT
    "../src/external/kissfft/kiss_fft.c"
)

# 컴파일
g++ -std=c++11 "${CPP_FILES[@]}" \
    -o benchmark_pitchshift \
    -I../src \
    -I../src/external/soundtouch/include \
    -I../src/external/soundtouch/source \
    -I../src/external/kissfft \
    -O3

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Run with: ./benchmark_pitchshift <input.wav>"
    echo "Example: ./benchmark_pitchshift ../original.wav"
else
    echo "✗ Build failed!"
    exit 1
fi
