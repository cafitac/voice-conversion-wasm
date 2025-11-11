# 테스트 프로그램들

# Pitch 분석 테스트
add_executable(test_pitch_analyzer
    test_pitch_analyzer.cpp
    ../src/audio/AudioBuffer.cpp
    ../src/analysis/PitchAnalyzer.cpp
)

# FrameData 재구성 테스트
add_executable(test_reconstruction
    test_reconstruction.cpp
    ../src/audio/AudioBuffer.cpp
    ../src/audio/AudioPreprocessor.cpp
    ../src/effects/PitchShifter.cpp
    ../src/effects/TimeStretcher.cpp
    ../src/effects/FramePitchModifier.cpp
    ../src/effects/TimeScaleModifier.cpp
    ../src/effects/PhaseVocoder.cpp
    ../src/effects/PhaseVocoderPitchShifter.cpp
    ../src/synthesis/FrameReconstructor.cpp
    ../src/utils/FFTWrapper.cpp
    ../src/utils/WaveFile.cpp
    ../src/external/kissfft/kiss_fft.c
)

# 실행 파일을 tests 디렉토리에 출력
set_target_properties(test_pitch_analyzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(test_reconstruction PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
