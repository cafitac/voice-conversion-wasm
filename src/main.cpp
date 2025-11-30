#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

#include "audio/AudioBuffer.h"
#include "analysis/PitchAnalyzer.h"
#include "effects/VoiceFilter.h"
#include "effects/AudioReverser.h"

// 외부 라이브러리 직접 사용을 위한 헤더
#include <SoundTouch.h>
#include <rubberband/RubberBandStretcher.h>

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
 * 외부 라이브러리(SoundTouch, RubberBand)를 직접 사용하여 전체 오디오 일괄 처리
 *
 * @param dataPtr 오디오 데이터 포인터
 * @param length 오디오 길이 (샘플 수)
 * @param sampleRate 샘플레이트
 * @param pitchSemitones Pitch shift 양 (semitones, -12 ~ +12)
 * @param algorithm 알고리즘 선택 ("soundtouch", "rubberband")
 * @return 처리된 오디오 (Float32Array)
 */
emscripten::val applyUniformPitchShift(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    float pitchSemitones,
    const std::string& algorithm
) {
  // 1. 오디오 데이터 변환
  float* audioData = reinterpret_cast<float*>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // 2. AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // 3. 외부 라이브러리로 직접 처리 (프레임 변환 없음)
  AudioBuffer result(sampleRate, 1);

  if (algorithm == "rubberband") {
    // RubberBand 직접 사용
    RubberBand::RubberBandStretcher::Options options =
        RubberBand::RubberBandStretcher::OptionProcessOffline |
        RubberBand::RubberBandStretcher::OptionEngineFiner |
        RubberBand::RubberBandStretcher::OptionFormantPreserved;

    RubberBand::RubberBandStretcher stretcher(sampleRate, 1, options);

    // 버퍼 크기 사전 설정 (경고 방지)
    stretcher.setMaxProcessSize(samples.size());
    stretcher.setExpectedInputDuration(samples.size());

    double pitchScale = std::pow(2.0, pitchSemitones / 12.0);
    stretcher.setPitchScale(pitchScale);
    stretcher.setTimeRatio(1.0);

    // Study and process
    const float* inputPtr = samples.data();
    stretcher.study(&inputPtr, samples.size(), true);
    stretcher.process(&inputPtr, samples.size(), true);

    // Retrieve output
    std::vector<float> outputData;
    int available = stretcher.available();
    if (available > 0) {
      outputData.resize(available);
      float* outputPtr = outputData.data();
      stretcher.retrieve(&outputPtr, available);
    }
    result.setData(outputData);

  } else {
    // SoundTouch 직접 사용 (기본값)
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
  }

  // 4. Float32Array로 변환하여 반환
  const auto& resultData = result.getData();
  val outputArray = val::global("Float32Array").new_(resultData.size());

  for (size_t i = 0; i < resultData.size(); ++i) {
    outputArray.set(i, resultData[i]);
  }

  return outputArray;
}

/**
 * 전체 파일에 균일한 Time Stretch 적용 (음성 효과용)
 * 외부 라이브러리(SoundTouch, RubberBand)를 직접 사용하여 전체 오디오 일괄 처리
 *
 * @param dataPtr 오디오 데이터 포인터
 * @param length 오디오 길이 (샘플 수)
 * @param sampleRate 샘플레이트
 * @param durationRatio Time stretch 비율 (0.5 ~ 2.0, 1.0 = 변화 없음)
 * @param algorithm 알고리즘 선택 ("wsola", "soundtouch", "rubberband")
 * @return 처리된 오디오 (Float32Array)
 */
emscripten::val applyUniformTimeStretch(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    float durationRatio,
    const std::string& algorithm
) {
  // 1. 오디오 데이터 변환
  float* audioData = reinterpret_cast<float*>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // 2. AudioBuffer 생성
  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  // 3. 외부 라이브러리로 직접 처리 (프레임 변환 없음)
  AudioBuffer result(sampleRate, 1);

  if (algorithm == "rubberband") {
    // RubberBand 직접 사용
    RubberBand::RubberBandStretcher::Options options =
        RubberBand::RubberBandStretcher::OptionProcessOffline |
        RubberBand::RubberBandStretcher::OptionEngineFiner;

    RubberBand::RubberBandStretcher stretcher(sampleRate, 1, options);

    // 버퍼 크기 사전 설정 (경고 방지)
    stretcher.setMaxProcessSize(samples.size());
    stretcher.setExpectedInputDuration(samples.size());

    stretcher.setPitchScale(1.0);  // 피치 유지
    stretcher.setTimeRatio(durationRatio);

    // Study and process
    const float* inputPtr = samples.data();
    stretcher.study(&inputPtr, samples.size(), true);
    stretcher.process(&inputPtr, samples.size(), true);

    // Retrieve output
    std::vector<float> outputData;
    int available = stretcher.available();
    if (available > 0) {
      outputData.resize(available);
      float* outputPtr = outputData.data();
      stretcher.retrieve(&outputPtr, available);
    }
    result.setData(outputData);

  } else {
    // SoundTouch 직접 사용 (기본값: soundtouch 및 wsola)
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
  }

  // 4. Float32Array로 변환하여 반환
  const auto& resultData = result.getData();
  val outputArray = val::global("Float32Array").new_(resultData.size());

  for (size_t i = 0; i < resultData.size(); ++i) {
    outputArray.set(i, resultData[i]);
  }

  return outputArray;
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

// 오디오 역재생
val reverseAudio(uintptr_t dataPtr, int length, int sampleRate) {
  float *data = reinterpret_cast<float *>(dataPtr);
  std::vector<float> samples(data, data + length);

  AudioBuffer buffer(sampleRate, 1);
  buffer.setData(samples);

  AudioReverser reverser;
  AudioBuffer result = reverser.reverse(buffer);

  // Float32Array로 변환하여 반환
  const auto& resultData = result.getData();
  val outputArray = val::global("Float32Array").new_(resultData.size());

  for (size_t i = 0; i < resultData.size(); ++i) {
    outputArray.set(i, resultData[i]);
  }

  return outputArray;
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
}
