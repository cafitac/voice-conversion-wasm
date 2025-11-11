#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "audio/AudioBuffer.h"
#include "audio/AudioRecorder.h"
#include "audio/AudioProcessor.h"
#include "audio/AudioPreprocessor.h"
#include "analysis/PitchAnalyzer.h"
#include "analysis/DurationAnalyzer.h"
#include "analysis/PowerAnalyzer.h"
#include "effects/PhaseVocoderPitchShifter.h"
#include "effects/TimeStretcher.h"
#include "effects/VoiceFilter.h"
#include "effects/FramePitchModifier.h"
#include "effects/TimeScaleModifier.h"
#include "effects/IPitchShiftStrategy.h"
#include "effects/FastPitchShiftStrategy.h"
#include "effects/HighQualityPitchShiftStrategy.h"
#include "effects/ExternalPitchShiftStrategy.h"
#include "synthesis/FrameReconstructor.h"
#include "utils/WaveFile.h"
#include "visualization/CanvasRenderer.h"
#include "visualization/TrimController.h"

using namespace emscripten;

// 전역 레코더 인스턴스
static AudioRecorder* g_recorder = nullptr;

// 전역 Trim Controller
static TrimController* g_trimController = nullptr;

// 전역 Pitch Shift Strategy (기본값: HighQuality)
static IPitchShiftStrategy* g_pitchShiftStrategy = nullptr;
static bool g_strategyOwned = false;

// 초기화
void init() {
    if (!g_recorder) {
        // 48000Hz로 변경 (대부분의 브라우저 기본값)
        g_recorder = new AudioRecorder(48000, 1);
    }
    if (!g_trimController) {
        g_trimController = new TrimController();
    }
    if (!g_pitchShiftStrategy) {
        // 기본값: External (SoundTouch) 사용
        g_pitchShiftStrategy = new ExternalPitchShiftStrategy(true, false);
        g_strategyOwned = true;
    }
}

// Pitch Shift Strategy 설정
void setPitchShiftQuality(const std::string& quality) {
    // 기존 strategy 정리
    if (g_strategyOwned && g_pitchShiftStrategy) {
        delete g_pitchShiftStrategy;
    }

    // 새 strategy 생성
    if (quality == "fast") {
        g_pitchShiftStrategy = new FastPitchShiftStrategy();
        g_strategyOwned = true;
    } else if (quality == "high") {
        g_pitchShiftStrategy = new HighQualityPitchShiftStrategy(1024, 256);
        g_strategyOwned = true;
    } else if (quality == "external") {
        g_pitchShiftStrategy = new ExternalPitchShiftStrategy(true, false);  // Anti-aliasing ON, QuickSeek OFF
        g_strategyOwned = true;
    } else {
        // 기본값: External (SoundTouch)
        g_pitchShiftStrategy = new ExternalPitchShiftStrategy(true, false);
        g_strategyOwned = true;
    }
}

// 현재 사용 중인 strategy 이름 반환
std::string getPitchShiftQuality() {
    init();
    return g_pitchShiftStrategy ? g_pitchShiftStrategy->getName() : "None";
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

// Power 분석
val analyzePower(uintptr_t dataPtr, int length, int sampleRate, float frameSize) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    PowerAnalyzer analyzer;
    auto powers = analyzer.analyze(buffer, frameSize);

    // JavaScript 배열로 변환
    val result = val::array();
    for (const auto& power : powers) {
        val obj = val::object();
        obj.set("time", power.time);
        obj.set("rms", power.rms);
        obj.set("dbFS", power.dbFS);
        result.call<void>("push", obj);
    }

    return result;
}

// Pitch Shift 적용 (Strategy 패턴 사용)
val applyPitchShift(uintptr_t dataPtr, int length, int sampleRate, float semitones) {
    init(); // g_pitchShiftStrategy 초기화 확인

    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    // Strategy 패턴: UI에서 선택한 알고리즘 사용
    AudioBuffer result = g_pitchShiftStrategy->shiftPitch(buffer, semitones);

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

// Duration + Pitch 통합 시각화
void drawCombinedAnalysis(uintptr_t dataPtr, int length, int sampleRate, const std::string& canvasId) {
    float* data = reinterpret_cast<float*>(dataPtr);
    std::vector<float> samples(data, data + length);

    AudioBuffer buffer(sampleRate, 1);
    buffer.setData(samples);

    // 공통 전처리: 프레임 분할 및 VAD
    AudioPreprocessor preprocessor;
    auto frames = preprocessor.process(buffer, 0.02f, 0.01f, 0.02f);  // 20ms 프레임, 10ms hop, 0.02 VAD threshold

    // Duration 분석 (전처리된 프레임 사용)
    DurationAnalyzer durationAnalyzer;
    auto segments = durationAnalyzer.analyzeFrames(frames);

    // Pitch 분석 (전처리된 프레임 사용)
    PitchAnalyzer pitchAnalyzer;
    auto pitchPoints = pitchAnalyzer.analyzeFrames(frames, sampleRate);

    // Canvas에 그리기
    CanvasRenderer renderer;
    renderer.drawCombinedAnalysis(canvasId, segments, pitchPoints, sampleRate);
}

// Trim handles 그리기
void drawTrimHandles(const std::string& canvasId, float trimStart, float trimEnd, float maxTime) {
    CanvasRenderer renderer;
    renderer.drawTrimHandles(canvasId, trimStart, trimEnd, maxTime);
}

// Trim controller functions
void enableTrimMode(const std::string& canvasId, float maxTime) {
    if (g_trimController) {
        g_trimController->enable(canvasId, maxTime);
    }
}

void disableTrimMode() {
    if (g_trimController) {
        g_trimController->disable();
    }
}

void trimMouseDown(float mouseX, float canvasWidth) {
    if (g_trimController) {
        g_trimController->startDrag(mouseX, canvasWidth);
    }
}

void trimMouseMove(float mouseX, float canvasWidth) {
    if (g_trimController) {
        g_trimController->updateTrimPosition(mouseX, canvasWidth);
        g_trimController->render();
    }
}

void trimMouseUp() {
    if (g_trimController) {
        g_trimController->stopDrag();
    }
}

float getTrimStart() {
    return g_trimController ? g_trimController->getTrimStart() : 0.0f;
}

float getTrimEnd() {
    return g_trimController ? g_trimController->getTrimEnd() : 0.0f;
}

bool isTrimDragging() {
    return g_trimController ? g_trimController->isDragging() : false;
}

void resetTrimHandles() {
    if (g_trimController) {
        g_trimController->reset();
    }
}

// Emscripten 바인딩
EMSCRIPTEN_BINDINGS(audio_module) {
    // 기본 함수
    function("init", &init);
    function("startRecording", &startRecording);
    function("stopRecording", &stopRecording);
    function("addAudioData", &addAudioData);
    function("getRecordedAudioAsWav", &getRecordedAudioAsWav);

    // Pitch Shift Quality 설정
    function("setPitchShiftQuality", &setPitchShiftQuality);
    function("getPitchShiftQuality", &getPitchShiftQuality);

    // 분석 함수
    function("analyzePitch", &analyzePitch);
    function("analyzeDuration", &analyzeDuration);
    function("analyzePower", &analyzePower);

    // 효과 함수
    function("applyPitchShift", &applyPitchShift);
    function("applyTimeStretch", &applyTimeStretch);
    function("applyVoiceFilter", &applyVoiceFilter);
    function("normalizeAudio", &normalizeAudio);

    // 시각화 함수
    function("drawCombinedAnalysis", &drawCombinedAnalysis);
    function("drawTrimHandles", &drawTrimHandles);

    // Trim controller 함수
    function("enableTrimMode", &enableTrimMode);
    function("disableTrimMode", &disableTrimMode);
    function("trimMouseDown", &trimMouseDown);
    function("trimMouseMove", &trimMouseMove);
    function("trimMouseUp", &trimMouseUp);
    function("getTrimStart", &getTrimStart);
    function("getTrimEnd", &getTrimEnd);
    function("isTrimDragging", &isTrimDragging);
    function("resetTrimHandles", &resetTrimHandles);

    // FilterType enum
    enum_<FilterType>("FilterType")
        .value("LOW_PASS", FilterType::LOW_PASS)
        .value("HIGH_PASS", FilterType::HIGH_PASS)
        .value("BAND_PASS", FilterType::BAND_PASS)
        .value("ROBOT", FilterType::ROBOT)
        .value("ECHO", FilterType::ECHO)
        .value("REVERB", FilterType::REVERB);
}
