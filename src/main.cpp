#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "audio/AudioBuffer.h"
#include "audio/AudioRecorder.h"
#include "audio/AudioProcessor.h"
#include "analysis/PitchAnalyzer.h"
#include "analysis/DurationAnalyzer.h"
#include "effects/PitchShifter.h"
#include "effects/TimeStretcher.h"
#include "effects/VoiceFilter.h"
#include "utils/WaveFile.h"

using namespace emscripten;

// 전역 레코더 인스턴스
static AudioRecorder* g_recorder = nullptr;

// 초기화
void init() {
    if (!g_recorder) {
        // 48000Hz로 변경 (대부분의 브라우저 기본값)
        g_recorder = new AudioRecorder(48000, 1);
    }
}

// 녹음 시작
void startRecording() {
    init();
    g_recorder->startRecording();
}

// 녹음 중지
void stopRecording() {
    if (g_recorder) {
        g_recorder->stopRecording();
    }
}

// 오디오 데이터 추가 (JavaScript에서 호출)
void addAudioData(uintptr_t dataPtr, int length) {
    if (g_recorder) {
        g_recorder->addAudioData(dataPtr, length);
    }
}

// 녹음된 오디오를 WAV 파일로 저장
val getRecordedAudioAsWav() {
    if (!g_recorder) {
        return val::null();
    }

    const AudioBuffer& buffer = g_recorder->getRecordedAudio();
    WaveFile wavFile;
    auto wavData = wavFile.saveToMemory(buffer);

    // JavaScript로 Uint8Array 반환
    return val(typed_memory_view(wavData.size(), wavData.data()));
}

// Pitch 분석
val analyzePitch(uintptr_t dataPtr, int length, int sampleRate) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    PitchAnalyzer analyzer;
    auto pitchPoints = analyzer.analyze(buffer);

    // JavaScript 배열로 변환
    val result = val::array();
    for (const auto& point : pitchPoints) {
        val obj = val::object();
        obj.set("time", point.time);
        obj.set("frequency", point.frequency);
        obj.set("confidence", point.confidence);
        result.call<void>("push", obj);
    }

    return result;
}

// Duration 분석
val analyzeDuration(uintptr_t dataPtr, int length, int sampleRate) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    DurationAnalyzer analyzer;
    auto segments = analyzer.analyzeSegments(buffer);

    // JavaScript 배열로 변환
    val result = val::array();
    for (const auto& segment : segments) {
        val obj = val::object();
        obj.set("startTime", segment.startTime);
        obj.set("endTime", segment.endTime);
        obj.set("duration", segment.duration);
        obj.set("energy", segment.energy);
        result.call<void>("push", obj);
    }

    return result;
}

// Pitch Shift 적용
val applyPitchShift(uintptr_t dataPtr, int length, int sampleRate, float semitones) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    PitchShifter shifter;
    AudioBuffer result = shifter.shiftPitch(buffer, semitones);

    const auto& resultData = result.getData();
    return val(typed_memory_view(resultData.size(), resultData.data()));
}

// Time Stretch 적용
val applyTimeStretch(uintptr_t dataPtr, int length, int sampleRate, float ratio) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    TimeStretcher stretcher;
    AudioBuffer result = stretcher.stretch(buffer, ratio);

    const auto& resultData = result.getData();
    return val(typed_memory_view(resultData.size(), resultData.data()));
}

// 음성 필터 적용
val applyVoiceFilter(uintptr_t dataPtr, int length, int sampleRate, int filterType, float param1, float param2) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    VoiceFilter filter;
    FilterType type = static_cast<FilterType>(filterType);
    AudioBuffer result = filter.applyFilter(buffer, type, param1, param2);

    const auto& resultData = result.getData();
    return val(typed_memory_view(resultData.size(), resultData.data()));
}

// 오디오 정규화
val normalizeAudio(uintptr_t dataPtr, int length, int sampleRate) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    AudioProcessor::normalize(buffer);

    const auto& resultData = buffer.getData();
    return val(typed_memory_view(resultData.size(), resultData.data()));
}

// Emscripten 바인딩
EMSCRIPTEN_BINDINGS(audio_module) {
    // 기본 함수
    function("init", &init);
    function("startRecording", &startRecording);
    function("stopRecording", &stopRecording);
    function("addAudioData", &addAudioData);
    function("getRecordedAudioAsWav", &getRecordedAudioAsWav);

    // 분석 함수
    function("analyzePitch", &analyzePitch);
    function("analyzeDuration", &analyzeDuration);

    // 효과 함수
    function("applyPitchShift", &applyPitchShift);
    function("applyTimeStretch", &applyTimeStretch);
    function("applyVoiceFilter", &applyVoiceFilter);
    function("normalizeAudio", &normalizeAudio);

    // FilterType enum
    enum_<FilterType>("FilterType")
        .value("LOW_PASS", FilterType::LOW_PASS)
        .value("HIGH_PASS", FilterType::HIGH_PASS)
        .value("BAND_PASS", FilterType::BAND_PASS)
        .value("ROBOT", FilterType::ROBOT)
        .value("ECHO", FilterType::ECHO)
        .value("REVERB", FilterType::REVERB);
}
