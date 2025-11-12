#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include "audio/AudioBuffer.h"
#include "audio/AudioRecorder.h"
#include "audio/AudioProcessor.h"
#include "audio/AudioPreprocessor.h"
#include "analysis/PitchAnalyzer.h"
#include "analysis/DurationAnalyzer.h"
#include "analysis/PowerAnalyzer.h"
#include "analysis/QualityAnalyzer.h"
#include "effects/PhaseVocoderPitchShifter.h"
#include "effects/TimeStretcher.h"
#include "effects/VoiceFilter.h"
#include "effects/FramePitchModifier.h"
#include "effects/TimeScaleModifier.h"
#include "effects/IPitchShiftStrategy.h"
#include "effects/FastPitchShiftStrategy.h"
#include "effects/HighQualityPitchShiftStrategy.h"
#include "effects/ExternalPitchShiftStrategy.h"
#include "effects/ITimeStretchStrategy.h"
#include "effects/FastTimeStretchStrategy.h"
#include "effects/HighQualityTimeStretchStrategy.h"
#include "effects/ExternalTimeStretchStrategy.h"
#include "effects/HighQualityPerFrameEditor.h"
#include "effects/ExternalPerFrameEditor.h"
#include "synthesis/FrameReconstructor.h"
#include "utils/WaveFile.h"
#include "visualization/CanvasRenderer.h"
#include "visualization/TrimController.h"

using namespace emscripten;

// 전역 레코더 인스턴스
static AudioRecorder *g_recorder = nullptr;

// 전역 Trim Controller
static TrimController *g_trimController = nullptr;

// 전역 Pitch Shift Strategy (기본값: External)
static IPitchShiftStrategy *g_pitchShiftStrategy = nullptr;
static bool g_strategyOwned = false;

// 전역 Time Stretch Strategy (기본값: External)
static ITimeStretchStrategy *g_timeStretchStrategy = nullptr;
static bool g_timeStretchStrategyOwned = false;

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
  if (!g_timeStretchStrategy) {
    // 기본값: External (SoundTouch) 사용
    g_timeStretchStrategy = new ExternalTimeStretchStrategy(true, false);
    g_timeStretchStrategyOwned = true;
  }
}

// Pitch Shift Strategy 설정
void setPitchShiftQuality(const std::string &quality) {
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
    g_pitchShiftStrategy = new ExternalPitchShiftStrategy(true, false);
    // Anti-aliasing ON, QuickSeek OFF
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

// Time Stretch Strategy 설정
void setTimeStretchQuality(const std::string &quality) {
  // 기존 strategy 정리
  if (g_timeStretchStrategyOwned && g_timeStretchStrategy) {
    delete g_timeStretchStrategy;
  }

  // 새 strategy 생성
  if (quality == "fast") {
    g_timeStretchStrategy = new FastTimeStretchStrategy();
    g_timeStretchStrategyOwned = true;
  } else if (quality == "high") {
    g_timeStretchStrategy = new HighQualityTimeStretchStrategy(1024, 256);
    g_timeStretchStrategyOwned = true;
  } else if (quality == "external") {
    g_timeStretchStrategy = new ExternalTimeStretchStrategy(true, false);
    // Anti-aliasing ON, QuickSeek OFF
    g_timeStretchStrategyOwned = true;
  } else {
    // 기본값: External (SoundTouch)
    g_timeStretchStrategy = new ExternalTimeStretchStrategy(true, false);
    g_timeStretchStrategyOwned = true;
  }
}

// 현재 사용 중인 time stretch strategy 이름 반환
std::string getTimeStretchQuality() {
  init();
  return g_timeStretchStrategy ? g_timeStretchStrategy->getName() : "None";
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

  const AudioBuffer &buffer = g_recorder->getRecordedAudio();
  WaveFile wavFile;
  auto wavData = wavFile.saveToMemory(buffer);

  // JavaScript로 Uint8Array 반환
  return val(typed_memory_view(wavData.size(), wavData.data()));
}

// Pitch 분석
val analyzePitch(uintptr_t dataPtr, int length, int sampleRate) {
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  PitchAnalyzer analyzer;
  auto pitchPoints = analyzer.analyze(buffer);

  // JavaScript 배열로 변환
  val result = val::array();
  for (const auto &point: pitchPoints) {
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
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  DurationAnalyzer analyzer;
  auto segments = analyzer.analyzeSegments(buffer);

  // JavaScript 배열로 변환
  val result = val::array();
  for (const auto &segment: segments) {
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
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  PowerAnalyzer analyzer;
  auto powers = analyzer.analyze(buffer, frameSize);

  // JavaScript 배열로 변환
  val result = val::array();
  for (const auto &power: powers) {
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

  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // Strategy 패턴: UI에서 선택한 알고리즘 사용
  AudioBuffer result = g_pitchShiftStrategy->shiftPitch(buffer, semitones);

  const auto &resultData = result.getData();
  return val(typed_memory_view(resultData.size(), resultData.data()));
}

// Time Stretch 적용
val applyTimeStretch(uintptr_t dataPtr,
                     int length,
                     int sampleRate,
                     float ratio) {
  init();

  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // Strategy 패턴: UI에서 선택한 알고리즘 사용
  AudioBuffer result = g_timeStretchStrategy->stretch(buffer, ratio);

  const auto &resultData = result.getData();
  return val(typed_memory_view(resultData.size(), resultData.data()));
}

// 음성 필터 적용
val applyVoiceFilter(uintptr_t dataPtr,
                     int length,
                     int sampleRate,
                     int filterType,
                     float param1,
                     float param2) {
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  VoiceFilter filter;
  FilterType type = static_cast<FilterType>(filterType);
  AudioBuffer result = filter.applyFilter(buffer, type, param1, param2);

  const auto &resultData = result.getData();
  return val(typed_memory_view(resultData.size(), resultData.data()));
}

// 오디오 정규화
val normalizeAudio(uintptr_t dataPtr, int length, int sampleRate) {
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  AudioProcessor::normalize(buffer);

  const auto &resultData = buffer.getData();
  return val(typed_memory_view(resultData.size(), resultData.data()));
}

// Duration + Pitch 통합 시각화
void drawCombinedAnalysis(uintptr_t dataPtr,
                          int length,
                          int sampleRate,
                          const std::string &canvasId) {
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // 공통 전처리: 프레임 분할 및 VAD
  AudioPreprocessor preprocessor;
  auto frames = preprocessor.process(buffer, 0.02f, 0.01f, 0.02f);
  // 20ms 프레임, 10ms hop, 0.02 VAD threshold

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
void drawTrimHandles(const std::string &canvasId,
                     float trimStart,
                     float trimEnd,
                     float maxTime) {
  CanvasRenderer renderer;
  renderer.drawTrimHandles(canvasId, trimStart, trimEnd, maxTime);
}

// Trim controller functions
void enableTrimMode(const std::string &canvasId, float maxTime) {
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

// FrameData를 JS Array로 변환하여 반환
emscripten::val getFrameDataArray(uintptr_t dataPtr, int length, int sampleRate) {
  init();

  // Float32Array를 std::vector로 변환
  float *audioData = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // AudioPreprocessor로 프레임 분할
  AudioPreprocessor preprocessor;
  std::vector<FrameData> frames = preprocessor.process(buffer, 0.042f, 0.021f);  // ~2048 samples at 48kHz

  // PitchAnalyzer로 각 프레임의 pitch 분석
  PitchAnalyzer pitchAnalyzer;

  // JS Array 생성
  emscripten::val jsArray = emscripten::val::array();

  for (size_t i = 0; i < frames.size(); ++i) {
    const FrameData &frame = frames[i];

    // 각 프레임의 pitch 분석
    PitchResult pitchResult = pitchAnalyzer.extractPitch(frame.samples, sampleRate);

    // JS Object 생성
    emscripten::val jsObject = emscripten::val::object();
    jsObject.set("frameIndex", static_cast<int>(i));
    jsObject.set("time", frame.time);
    jsObject.set("pitch", pitchResult.frequency);
    jsObject.set("rms", frame.rms);
    jsObject.set("isVoice", frame.isVoice);

    // Array에 추가
    jsArray.call<void>("push", jsObject);
  }

  return jsArray;
}

// HighQuality 파이프라인으로 편집 적용
emscripten::val applyEditsHighQuality(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    emscripten::val pitchEditsArray,
    emscripten::val durationRegionsArray
) {
  init();

  // Float32Array를 std::vector로 변환
  float *audioData = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // AudioPreprocessor로 프레임 분할
  AudioPreprocessor preprocessor;
  std::vector<FrameData> frames = preprocessor.process(buffer, 0.042f, 0.021f);

  // JS Array에서 Pitch Edits 파싱
  std::vector<float> pitchShiftSemitones(frames.size(), 0.0f);
  int pitchEditsLength = pitchEditsArray["length"].as<int>();

  for (int i = 0; i < pitchEditsLength; ++i) {
    emscripten::val edit = pitchEditsArray[i];
    int frameIndex = edit["frameIndex"].as<int>();
    float semitones = edit["semitones"].as<float>();

    if (frameIndex >= 0 && frameIndex < static_cast<int>(pitchShiftSemitones.size())) {
      pitchShiftSemitones[frameIndex] = semitones;
    }
  }

  // JS Array에서 Duration Regions 파싱
  std::vector<TimeStretchRegion> regions;
  int regionsLength = durationRegionsArray["length"].as<int>();

  for (int i = 0; i < regionsLength; ++i) {
    emscripten::val region = durationRegionsArray[i];
    TimeStretchRegion r;
    r.startFrame = region["startFrame"].as<int>();
    r.endFrame = region["endFrame"].as<int>();
    r.ratio = region["ratio"].as<float>();
    regions.push_back(r);
  }

  // HighQualityPerFrameEditor로 편집 적용
  HighQualityPerFrameEditor editor;
  AudioBuffer result;

  if (!regions.empty() && pitchEditsLength > 0) {
    // Pitch + Duration 통합 편집
    result = editor.applyAllEdits(frames, pitchShiftSemitones, regions, sampleRate, 1);
  } else if (!regions.empty()) {
    // Duration만 편집
    result = editor.applyDurationEdits(frames, regions, sampleRate, 1);
  } else if (pitchEditsLength > 0) {
    // Pitch만 편집
    result = editor.applyPitchEdits(frames, pitchShiftSemitones, sampleRate, 1);
  } else {
    // 편집 없음 - 원본 그대로
    result = AudioBuffer(sampleRate, 1);
    result.setData(samples);
  }

  // Float32Array로 변환하여 반환
  const std::vector<float> &outputData = result.getData();
  emscripten::val outputArray = emscripten::val::global("Float32Array").new_(outputData.size());

  for (size_t i = 0; i < outputData.size(); ++i) {
    outputArray.set(i, outputData[i]);
  }

  return outputArray;
}

// HighQuality 파이프라인으로 편집 적용 (Key Points 사용)
emscripten::val applyEditsHighQualityWithKeyPoints(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    emscripten::val keyPointsArray,
    emscripten::val durationRegionsArray
) {
  init();

  // Float32Array를 std::vector로 변환
  float *audioData = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // AudioPreprocessor로 프레임 분할
  AudioPreprocessor preprocessor;
  std::vector<FrameData> frames = preprocessor.process(buffer, 0.042f, 0.021f);

  // JS Array에서 Key Points 파싱
  std::vector<PitchKeyPoint> keyPoints;
  int keyPointsLength = keyPointsArray["length"].as<int>();

  for (int i = 0; i < keyPointsLength; ++i) {
    emscripten::val kp = keyPointsArray[i];
    PitchKeyPoint point;
    point.frameIndex = kp["frameIndex"].as<int>();
    point.semitones = kp["semitones"].as<float>();
    keyPoints.push_back(point);
  }

  // JS Array에서 Duration Regions 파싱
  std::vector<TimeStretchRegion> regions;
  int regionsLength = durationRegionsArray["length"].as<int>();

  for (int i = 0; i < regionsLength; ++i) {
    emscripten::val region = durationRegionsArray[i];
    TimeStretchRegion r;
    r.startFrame = region["startFrame"].as<int>();
    r.endFrame = region["endFrame"].as<int>();
    r.ratio = region["ratio"].as<float>();
    regions.push_back(r);
  }

  // HighQualityPerFrameEditor로 편집 적용
  HighQualityPerFrameEditor editor;
  AudioBuffer result;

  if (!regions.empty() && !keyPoints.empty()) {
    // Pitch (key points) + Duration 통합 편집
    // Duration은 아직 key points를 지원하지 않으므로 pitch만 key points 사용
    AudioBuffer pitchEdited = editor.applyPitchEditsWithKeyPoints(frames, keyPoints, sampleRate, 1);
    // Duration은 별도 처리 필요 (TODO: 통합)
    result = pitchEdited;
  } else if (!regions.empty()) {
    // Duration만 편집
    result = editor.applyDurationEdits(frames, regions, sampleRate, 1);
  } else if (!keyPoints.empty()) {
    // Pitch만 편집 (key points 사용)
    result = editor.applyPitchEditsWithKeyPoints(frames, keyPoints, sampleRate, 1);
  } else {
    // 편집 없음 - 원본 그대로
    result = AudioBuffer(sampleRate, 1);
    result.setData(samples);
  }

  // Float32Array로 변환하여 반환
  const std::vector<float> &outputData = result.getData();
  emscripten::val outputArray = emscripten::val::global("Float32Array").new_(outputData.size());

  for (size_t i = 0; i < outputData.size(); ++i) {
    outputArray.set(i, outputData[i]);
  }

  return outputArray;
}

// External 파이프라인으로 편집 적용 (Key Points 사용)
emscripten::val applyEditsExternalWithKeyPoints(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    emscripten::val keyPointsArray,
    emscripten::val durationRegionsArray
) {
  init();

  // Float32Array를 std::vector로 변환
  float *audioData = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // AudioPreprocessor로 프레임 분할
  AudioPreprocessor preprocessor;
  std::vector<FrameData> frames = preprocessor.process(buffer, 0.042f, 0.021f);

  // JS Array에서 Key Points 파싱
  std::vector<PitchKeyPoint> keyPoints;
  int keyPointsLength = keyPointsArray["length"].as<int>();

  for (int i = 0; i < keyPointsLength; ++i) {
    emscripten::val kp = keyPointsArray[i];
    PitchKeyPoint point;
    point.frameIndex = kp["frameIndex"].as<int>();
    point.semitones = kp["semitones"].as<float>();
    keyPoints.push_back(point);
  }

  // JS Array에서 Duration Regions 파싱
  std::vector<TimeStretchRegion> regions;
  int regionsLength = durationRegionsArray["length"].as<int>();

  for (int i = 0; i < regionsLength; ++i) {
    emscripten::val region = durationRegionsArray[i];
    TimeStretchRegion r;
    r.startFrame = region["startFrame"].as<int>();
    r.endFrame = region["endFrame"].as<int>();
    r.ratio = region["ratio"].as<float>();
    regions.push_back(r);
  }

  // ExternalPerFrameEditor로 편집 적용 (Hybrid)
  ExternalPerFrameEditor editor;
  AudioBuffer result;

  if (!regions.empty() && !keyPoints.empty()) {
    // Pitch (key points) + Duration 통합 편집
    // Duration은 아직 key points를 지원하지 않으므로 pitch만 key points 사용
    AudioBuffer pitchEdited = editor.applyPitchEditsWithKeyPoints(frames, keyPoints, sampleRate, 1);
    // Duration은 별도 처리 필요 (TODO: 통합)
    result = pitchEdited;
  } else if (!regions.empty()) {
    // Duration만 편집
    result = editor.applyDurationEdits(frames, regions, sampleRate, 1);
  } else if (!keyPoints.empty()) {
    // Pitch만 편집 (key points 사용)
    result = editor.applyPitchEditsWithKeyPoints(frames, keyPoints, sampleRate, 1);
  } else {
    // 편집 없음 - 원본 그대로
    result = AudioBuffer(sampleRate, 1);
    result.setData(samples);
  }

  // Float32Array로 변환하여 반환
  const std::vector<float> &outputData = result.getData();
  emscripten::val outputArray = emscripten::val::global("Float32Array").new_(outputData.size());

  for (size_t i = 0; i < outputData.size(); ++i) {
    outputArray.set(i, outputData[i]);
  }

  return outputArray;
}

// External 파이프라인으로 편집 적용 (Hybrid: SoundTouch + 자체 알고리즘)
emscripten::val applyEditsExternal(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    emscripten::val pitchEditsArray,
    emscripten::val durationRegionsArray
) {
  init();

  // Float32Array를 std::vector로 변환
  float *audioData = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // AudioPreprocessor로 프레임 분할
  AudioPreprocessor preprocessor;
  std::vector<FrameData> frames = preprocessor.process(buffer, 0.042f, 0.021f);

  // JS Array에서 Pitch Edits 파싱
  std::vector<float> pitchShiftSemitones(frames.size(), 0.0f);
  int pitchEditsLength = pitchEditsArray["length"].as<int>();

  for (int i = 0; i < pitchEditsLength; ++i) {
    emscripten::val edit = pitchEditsArray[i];
    int frameIndex = edit["frameIndex"].as<int>();
    float semitones = edit["semitones"].as<float>();

    if (frameIndex >= 0 && frameIndex < static_cast<int>(pitchShiftSemitones.size())) {
      pitchShiftSemitones[frameIndex] = semitones;
    }
  }

  // JS Array에서 Duration Regions 파싱
  std::vector<TimeStretchRegion> regions;
  int regionsLength = durationRegionsArray["length"].as<int>();

  for (int i = 0; i < regionsLength; ++i) {
    emscripten::val region = durationRegionsArray[i];
    TimeStretchRegion r;
    r.startFrame = region["startFrame"].as<int>();
    r.endFrame = region["endFrame"].as<int>();
    r.ratio = region["ratio"].as<float>();
    regions.push_back(r);
  }

  // ExternalPerFrameEditor로 편집 적용 (Hybrid)
  ExternalPerFrameEditor editor;
  AudioBuffer result;

  if (!regions.empty() && pitchEditsLength > 0) {
    // Pitch + Duration 통합 편집
    result = editor.applyAllEdits(frames, pitchShiftSemitones, regions, sampleRate, 1);
  } else if (!regions.empty()) {
    // Duration만 편집
    result = editor.applyDurationEdits(frames, regions, sampleRate, 1);
  } else if (pitchEditsLength > 0) {
    // Pitch만 편집
    result = editor.applyPitchEdits(frames, pitchShiftSemitones, sampleRate, 1);
  } else {
    // 편집 없음 - 원본 그대로
    result = AudioBuffer(sampleRate, 1);
    result.setData(samples);
  }

  // Float32Array로 변환하여 반환
  const std::vector<float> &outputData = result.getData();
  emscripten::val outputArray = emscripten::val::global("Float32Array").new_(outputData.size());

  for (size_t i = 0; i < outputData.size(); ++i) {
    outputArray.set(i, outputData[i]);
  }

  return outputArray;
}

// Emscripten 바인딩
/**
 * 품질 분석 함수
 * 원본과 처리된 오디오를 비교하여 품질 메트릭을 반환합니다.
 */
emscripten::val analyzeQuality(
    uintptr_t originalPtr, int originalLength,
    uintptr_t processedPtr, int processedLength,
    int sampleRate
) {
  init();

  // 포인터를 float 배열로 변환
  float* originalData = reinterpret_cast<float*>(originalPtr);
  float* processedData = reinterpret_cast<float*>(processedPtr);

  // std::vector로 변환
  std::vector<float> originalSamples(originalData, originalData + originalLength);
  std::vector<float> processedSamples(processedData, processedData + processedLength);

  // QualityAnalyzer로 분석
  QualityAnalyzer analyzer;
  QualityMetrics metrics = analyzer.analyze(originalSamples, processedSamples, sampleRate);

  // JavaScript 객체로 변환
  emscripten::val result = emscripten::val::object();
  result.set("snr", metrics.snr);
  result.set("rmsError", metrics.rmsError);
  result.set("peakError", metrics.peakError);
  result.set("thd", metrics.thd);
  result.set("spectralDistortion", metrics.spectralDistortion);
  result.set("correlation", metrics.correlation);
  result.set("processingTime", metrics.processingTime);

  return result;
}

EMSCRIPTEN_BINDINGS (audio_module) {
  // 기본 함수
  function("init", &init);
  function("startRecording", &startRecording);
  function("stopRecording", &stopRecording);
  function("addAudioData", &addAudioData);
  function("getRecordedAudioAsWav", &getRecordedAudioAsWav);

  // Pitch Shift Quality 설정
  function("setPitchShiftQuality", &setPitchShiftQuality);
  function("getPitchShiftQuality", &getPitchShiftQuality);

  // Time Stretch Quality 설정
  function("setTimeStretchQuality", &setTimeStretchQuality);
  function("getTimeStretchQuality", &getTimeStretchQuality);

  // 분석 함수
  function("analyzePitch", &analyzePitch);
  function("analyzeDuration", &analyzeDuration);
  function("analyzePower", &analyzePower);
  function("analyzeQuality", &analyzeQuality);

  // 인터랙티브 편집용 함수
  function("getFrameDataArray", &getFrameDataArray);
  function("applyEditsHighQuality", &applyEditsHighQuality);
  function("applyEditsExternal", &applyEditsExternal);

  // 인터랙티브 편집용 함수 (Key Points 사용)
  function("applyEditsHighQualityWithKeyPoints", &applyEditsHighQualityWithKeyPoints);
  function("applyEditsExternalWithKeyPoints", &applyEditsExternalWithKeyPoints);

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
