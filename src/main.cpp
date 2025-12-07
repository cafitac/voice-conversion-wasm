#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include "audio/AudioBuffer.h"
#include "analysis/PitchAnalyzer.h"
#include "effects/VoiceFilter.h"
#include "effects/AudioReverser.h"
#include "performance/PerformanceChecker.h"

// 직접 구현한 DSP 알고리즘
#include "dsp/SimplePitchShifter.h"
#include "dsp/SimpleTimeStretcher.h"

// 외부 라이브러리 (비교용으로 남겨둠)
#include <SoundTouch.h>

using namespace emscripten;

// 초기화
void init() {
  // 필요 시 초기화 작업 수행
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

/**
 * 전체 파일에 균일한 Pitch Shift 적용 (음성 효과용)
 * 직접 구현한 SimplePitchShifter 사용
 *
 * @param dataPtr 오디오 데이터 포인터
 * @param length 오디오 길이 (샘플 수)
 * @param sampleRate 샘플레이트
 * @param pitchSemitones Pitch shift 양 (semitones, -12 ~ +12)
 * @param algorithm 알고리즘 선택 ("simple" 또는 "soundtouch")
 * @param perfCheckerVal PerformanceChecker 객체 (optional)
 * @return 처리된 오디오 (Float32Array)
 */
emscripten::val applyUniformPitchShift(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    float pitchSemitones,
    const std::string& algorithm,
    val perfCheckerVal = val::null()
) {
  // 1. 오디오 데이터 변환
  float* audioData = reinterpret_cast<float*>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // 2. AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  AudioBuffer result(sampleRate, 1);

  // PerformanceChecker 가져오기 (옵션)
  PerformanceChecker* perfChecker = nullptr;
  if (!perfCheckerVal.isNull() && !perfCheckerVal.isUndefined()) {
    perfChecker = &perfCheckerVal.as<PerformanceChecker&>();
  }

  if (algorithm == "soundtouch") {
    // 기존 SoundTouch 사용 (비교용)
    soundtouch::SoundTouch st;
    st.setSampleRate(sampleRate);
    st.setChannels(1);
    st.setPitchSemiTones(pitchSemitones);
    st.setTempo(1.0f);  // 속도 유지
    st.setSetting(SETTING_USE_AA_FILTER, 1);
    st.setSetting(SETTING_AA_FILTER_LENGTH, 64);
    st.setSetting(SETTING_SEQUENCE_MS, 40);
    st.setSetting(SETTING_SEEKWINDOW_MS, 15);
    st.setSetting(SETTING_OVERLAP_MS, 8);

    // Process
    st.putSamples(samples.data(), samples.size());
    st.flush();

    // Retrieve output
    std::vector<float> outputData;
    outputData.resize(samples.size() * 2);  // 여유 공간
    int received = st.receiveSamples(outputData.data(), outputData.size());
    outputData.resize(received);
    result.setData(outputData);
  } else {
    // 3. 직접 구현한 SimplePitchShifter 사용 (기본값)
    SimplePitchShifter pitchShifter;
    result = pitchShifter.process(buffer, pitchSemitones, perfChecker);
  }

  // 4. Float32Array로 변환하여 반환 (Zero-copy: 메모리 직접 참조)
  const auto& resultData = result.getData();
  return val(typed_memory_view(resultData.size(), resultData.data()));
}

/**
 * 전체 파일에 균일한 Time Stretch 적용 (음성 효과용)
 * 직접 구현한 SimpleTimeStretcher 사용
 *
 * @param dataPtr 오디오 데이터 포인터
 * @param length 오디오 길이 (샘플 수)
 * @param sampleRate 샘플레이트
 * @param durationRatio Time stretch 비율 (0.5 ~ 2.0, 1.0 = 변화 없음)
 * @param algorithm 알고리즘 선택 ("simple" 또는 "soundtouch")
 * @param perfCheckerVal PerformanceChecker 객체 (optional)
 * @return 처리된 오디오 (Float32Array)
 */
emscripten::val applyUniformTimeStretch(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    float durationRatio,
    const std::string& algorithm,
    val perfCheckerVal = val::null()
) {
  // 1. 오디오 데이터 변환
  float* audioData = reinterpret_cast<float*>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // 2. AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  AudioBuffer result(sampleRate, 1);

  // PerformanceChecker 가져오기 (옵션)
  PerformanceChecker* perfChecker = nullptr;
  if (!perfCheckerVal.isNull() && !perfCheckerVal.isUndefined()) {
    perfChecker = &perfCheckerVal.as<PerformanceChecker&>();
  }

  if (algorithm == "soundtouch") {
    // 기존 SoundTouch 사용 (비교용)
    soundtouch::SoundTouch st;
    st.setSampleRate(sampleRate);
    st.setChannels(1);
    st.setPitchSemiTones(0.0f);  // 피치 유지
    st.setTempo(durationRatio);
    st.setSetting(SETTING_USE_AA_FILTER, 1);
    st.setSetting(SETTING_AA_FILTER_LENGTH, 64);
    st.setSetting(SETTING_SEQUENCE_MS, 40);
    st.setSetting(SETTING_SEEKWINDOW_MS, 15);
    st.setSetting(SETTING_OVERLAP_MS, 8);

    // Process
    st.putSamples(samples.data(), samples.size());
    st.flush();

    // Retrieve output - 예상 출력 크기 계산
    size_t expectedSize = static_cast<size_t>(samples.size() / durationRatio) + 8192;
    std::vector<float> outputData;
    outputData.resize(expectedSize);
    int received = st.receiveSamples(outputData.data(), outputData.size());
    outputData.resize(received);
    result.setData(outputData);
  } else {
    // 3. 직접 구현한 SimpleTimeStretcher 사용 (기본값)
    SimpleTimeStretcher timeStretcher;
    result = timeStretcher.process(buffer, durationRatio, perfChecker);
  }

  // 4. Float32Array로 변환하여 반환 (Zero-copy: 메모리 직접 참조)
  const auto& resultData = result.getData();
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

/**
 * InPlace Pitch Shift: 출력 버퍼를 JS에서 미리 할당하여 복사 완전 제거
 * @param inputPtr 입력 오디오 포인터
 * @param outputPtr 출력 오디오 포인터 (JS에서 미리 할당)
 * @param length 입력 길이
 * @param outputLength 출력 길이 (JS에서 계산)
 * @param sampleRate 샘플레이트
 * @param pitchSemitones Pitch shift 양
 * @return 실제 출력된 샘플 수
 */
int applyUniformPitchShiftInPlace(
    uintptr_t inputPtr,
    uintptr_t outputPtr,
    int length,
    int outputLength,
    int sampleRate,
    float pitchSemitones
) {
  float* inputData = reinterpret_cast<float*>(inputPtr);
  float* outputData = reinterpret_cast<float*>(outputPtr);

  std::vector<float> samples(inputData, inputData + length);
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  SimplePitchShifter pitchShifter;
  AudioBuffer result = pitchShifter.process(buffer, pitchSemitones, nullptr);

  const auto& resultData = result.getData();
  int copyLength = std::min((int)resultData.size(), outputLength);

  // 출력 버퍼에 직접 복사
  std::memcpy(outputData, resultData.data(), copyLength * sizeof(float));

  return copyLength;
}

/**
 * InPlace Time Stretch: 출력 버퍼를 JS에서 미리 할당하여 복사 완전 제거
 */
int applyUniformTimeStretchInPlace(
    uintptr_t inputPtr,
    uintptr_t outputPtr,
    int length,
    int outputLength,
    int sampleRate,
    float durationRatio
) {
  float* inputData = reinterpret_cast<float*>(inputPtr);
  float* outputData = reinterpret_cast<float*>(outputPtr);

  std::vector<float> samples(inputData, inputData + length);
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  SimpleTimeStretcher timeStretcher;
  AudioBuffer result = timeStretcher.process(buffer, durationRatio, nullptr);

  const auto& resultData = result.getData();
  int copyLength = std::min((int)resultData.size(), outputLength);

  // 출력 버퍼에 직접 복사
  std::memcpy(outputData, resultData.data(), copyLength * sizeof(float));

  return copyLength;
}

// 오디오 역재생
val reverseAudio(uintptr_t dataPtr, int length, int sampleRate) {
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  AudioReverser reverser;
  AudioBuffer result = reverser.reverse(buffer);

  // Float32Array로 변환하여 반환 (Zero-copy: 메모리 직접 참조)
  const auto& resultData = result.getData();
  return val(typed_memory_view(resultData.size(), resultData.data()));
}

// Emscripten 바인딩
EMSCRIPTEN_BINDINGS(audio_module) {
  // 초기화
  function("init", &init);

  // 분석 함수
  function("analyzePitch", &analyzePitch);

  // 효과 함수
  function("applyUniformPitchShift", &applyUniformPitchShift);
  function("applyUniformTimeStretch", &applyUniformTimeStretch);
  function("applyVoiceFilter", &applyVoiceFilter);
  function("reverseAudio", &reverseAudio);

  // InPlace 효과 함수 (Zero-copy 최적화)
  function("applyUniformPitchShiftInPlace", &applyUniformPitchShiftInPlace);
  function("applyUniformTimeStretchInPlace", &applyUniformTimeStretchInPlace);

  // FilterType enum
  enum_<FilterType>("FilterType")
      .value("LOW_PASS", FilterType::LOW_PASS)
      .value("HIGH_PASS", FilterType::HIGH_PASS)
      .value("BAND_PASS", FilterType::BAND_PASS)
      .value("ROBOT", FilterType::ROBOT)
      .value("ECHO", FilterType::ECHO)
      .value("REVERB", FilterType::REVERB)
      .value("DISTORTION", FilterType::DISTORTION)
      .value("AM_RADIO", FilterType::AM_RADIO)
      .value("CHORUS", FilterType::CHORUS)
      .value("FLANGER", FilterType::FLANGER)
      .value("VOICE_CHANGER_MALE_TO_FEMALE", FilterType::VOICE_CHANGER_MALE_TO_FEMALE)
      .value("VOICE_CHANGER_FEMALE_TO_MALE", FilterType::VOICE_CHANGER_FEMALE_TO_MALE);

  // PerformanceChecker FunctionNode
  value_object<PerformanceChecker::FunctionNode>("FunctionNode")
      .field("name", &PerformanceChecker::FunctionNode::name)
      .field("duration", &PerformanceChecker::FunctionNode::duration)
      .field("children", &PerformanceChecker::FunctionNode::children);

  // PerformanceChecker FeatureNode
  value_object<PerformanceChecker::FeatureNode>("FeatureNode")
      .field("feature", &PerformanceChecker::FeatureNode::feature)
      .field("duration", &PerformanceChecker::FeatureNode::duration)
      .field("functions", &PerformanceChecker::FeatureNode::functions);

  // PerformanceChecker class
  class_<PerformanceChecker>("PerformanceChecker")
      .constructor<>()
      .function("start", &PerformanceChecker::start)
      .function("end", &PerformanceChecker::end)
      .function("getAverage", &PerformanceChecker::getAverage)
      .function("reset", &PerformanceChecker::reset)
      .function("getReportJSON", &PerformanceChecker::getReportJSON)
      .function("getReportCSV", &PerformanceChecker::getReportCSV)
      .function("startFeature", &PerformanceChecker::startFeature)
      .function("endFeature", &PerformanceChecker::endFeature)
      .function("startFunction", &PerformanceChecker::startFunction)
      .function("endFunction", &PerformanceChecker::endFunction)
      .function("getFeatures", &PerformanceChecker::getFeatures)
      .function("getTotalDuration", &PerformanceChecker::getTotalDuration);

  // Vector types
  register_vector<PerformanceChecker::FunctionNode>("VectorFunctionNode");
  register_vector<PerformanceChecker::FeatureNode>("VectorFeatureNode");
}
