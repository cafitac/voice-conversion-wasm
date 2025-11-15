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
#include "effects/VoiceFilter.h"
#include "audio/FrameReconstructor.h"
#include "utils/WaveFile.h"
#include "utils/PitchCurveInterpolator.h"
#include "pipeline/IPipeline.h"
#include "pipeline/PitchFirstPipeline.h"
#include "pipeline/HybridPipeline.h"
#include "utils/EditPointManager.h"
#include "utils/EditPointGenerator.h"
#include "processor/pitch/IPitchProcessor.h"
#include "processor/pitch/PSOLAPitchProcessor.h"
#include "processor/pitch/PhaseVocoderPitchProcessor.h"
#include "processor/pitch/SoundTouchPitchProcessor.h"
#include "processor/pitch/RubberBandPitchProcessor.h"
#include "processor/duration/IDurationProcessor.h"
#include "processor/duration/WSOLADurationProcessor.h"
#include "processor/duration/SoundTouchDurationProcessor.h"
#include "processor/duration/RubberBandDurationProcessor.h"

using namespace emscripten;

// 전역 레코더 인스턴스
static AudioRecorder *g_recorder = nullptr;

// 전역 편집 포인트 매니저
static EditPointManager *g_editPointManager = nullptr;

// 초기화
void init() {
  if (!g_recorder) {
    // 48000Hz로 변경 (대부분의 브라우저 기본값)
    g_recorder = new AudioRecorder(48000, 1);
  }
  if (!g_editPointManager) {
    g_editPointManager = new EditPointManager();
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

// applyPitchShift, applyVariablePitchShift, applyTimeStretch 제거됨
// 새 Pipeline 아키텍처(preprocessAndInterpolate + processAudioWithPipeline) 사용

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

// applyEdits 함수들 모두 제거됨 (HighQuality, External, WithKeyPoints 버전)
// 새 Pipeline 아키텍처 사용: preprocessAndInterpolate + processAudioWithPipeline

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

/**
 * 편집 포인트 생성
 * - EditPointGenerator 클래스 사용
 * - 균등 배치 + 변곡점 자동 감지
 *
 * @param pitchDataJS PitchAnalyzer 결과
 * @param frameInterval 기본 간격 (프레임)
 * @param gradientThreshold 변곡점 감지 임계값 (Hz)
 * @param confidenceThreshold 최소 confidence
 * @return 편집 포인트 인덱스 배열
 */
emscripten::val generateEditPoints(
    emscripten::val pitchDataJS,
    int frameInterval = 10,
    float gradientThreshold = 50.0f,
    float confidenceThreshold = 0.3f
) {
  init();

  // 1. pitchData 파싱
  int numPoints = pitchDataJS["length"].as<int>();
  std::vector<PitchPoint> pitchData;
  for (int i = 0; i < numPoints; ++i) {
    emscripten::val point = pitchDataJS[i];
    PitchPoint p;
    p.time = point["time"].as<float>();
    p.frequency = point["frequency"].as<float>();
    p.confidence = point["confidence"].as<float>();
    pitchData.push_back(p);
  }

  // 2. EditPointGenerator로 편집 포인트 생성
  EditPointGenerator generator;
  std::vector<int> editIndices = generator.generate(
      pitchData, frameInterval, gradientThreshold, confidenceThreshold
  );

  // 3. JavaScript 배열로 변환
  emscripten::val result = emscripten::val::array();
  for (int idx : editIndices) {
    result.call<void>("push", idx);
  }

  return result;
}

/**
 * pitchData 처리 및 보간
 * - EditPointManager, SplineInterpolator 사용
 * - Semitones 기반 보간 (청각적으로 자연스러움)
 *
 * @param pitchDataJS 전체 pitchData [{time, frequency, confidence}]
 * @param changedIndex 방금 변경된 인덱스
 * @param allEditIndicesJS 모든 편집 포인트
 * @param gradientThreshold Outlier 감지 임계값
 * @return {pitchData: 수정된 배열, editIndices: 보정된 인덱스들}
 */
emscripten::val processPitchData(
    emscripten::val pitchDataJS,
    int changedIndex,  // 방금 변경된 인덱스 (1개)
    emscripten::val allEditIndicesJS,  // 모든 편집 포인트 (16개)
    float gradientThreshold = 3.0f
) {
  init();

  // 1. pitchData 파싱
  int numPoints = pitchDataJS["length"].as<int>();
  std::vector<PitchPoint> pitchData;
  for (int i = 0; i < numPoints; ++i) {
    emscripten::val point = pitchDataJS[i];
    PitchPoint p;
    p.time = point["time"].as<float>();
    p.frequency = point["frequency"].as<float>();
    p.confidence = point["confidence"].as<float>();
    pitchData.push_back(p);
  }

  // 2. 모든 편집 포인트 파싱
  int numEdits = allEditIndicesJS["length"].as<int>();
  std::vector<int> allEditIndices;
  for (int i = 0; i < numEdits; ++i) {
    allEditIndices.push_back(allEditIndicesJS[i].as<int>());
  }

  // 3. 변경된 인덱스에 대해서만 outlier correction 수행
  if (gradientThreshold > 0.0f && numPoints >= 3 && changedIndex >= 0) {
    int idx = changedIndex;
    if (idx > 0 && idx < numPoints - 1) {
      float prevFreq = pitchData[idx - 1].frequency;
      float currFreq = pitchData[idx].frequency;
      float nextFreq = pitchData[idx + 1].frequency;

      float prevTime = pitchData[idx - 1].time;
      float currTime = pitchData[idx].time;
      float nextTime = pitchData[idx + 1].time;

      float dt1 = currTime - prevTime;
      float dt2 = nextTime - currTime;

      if (dt1 > 0.0f && dt2 > 0.0f) {
        // semitones로 변환
        float prevSemitones = 12.0f * std::log2(prevFreq / 440.0f);
        float currSemitones = 12.0f * std::log2(currFreq / 440.0f);
        float nextSemitones = 12.0f * std::log2(nextFreq / 440.0f);

        float gradient1 = (currSemitones - prevSemitones) / dt1;
        float gradient2 = (nextSemitones - currSemitones) / dt2;

        // Outlier 감지
        if (std::abs(gradient1 - gradient2) > gradientThreshold) {
          // 선형 보간으로 보정
          float totalDt = nextTime - prevTime;
          float ratio = (currTime - prevTime) / totalDt;
          float correctedSemitones = prevSemitones + ratio * (nextSemitones - prevSemitones);
          float correctedFreq = 440.0f * std::pow(2.0f, correctedSemitones / 12.0f);
          pitchData[changedIndex].frequency = correctedFreq;
        }
      }
    }
  }

  // 4. 변경된 인덱스 주변만 cubic spline 보간 (이전 편집포인트 ~ 다음 편집포인트 구간만)
  if (changedIndex >= 0) {
    // allEditIndices를 정렬
    std::vector<int> sortedEditIndices = allEditIndices;
    std::sort(sortedEditIndices.begin(), sortedEditIndices.end());

    // 변경된 인덱스의 이전/다음 편집 포인트 찾기
    int prevEditIdx = -1;
    int nextEditIdx = numPoints;

    for (int ei : sortedEditIndices) {
      if (ei < changedIndex && ei > prevEditIdx) prevEditIdx = ei;
      if (ei > changedIndex && ei < nextEditIdx) nextEditIdx = ei;
    }

    if (prevEditIdx == -1) prevEditIdx = 0;
    if (nextEditIdx == numPoints) nextEditIdx = numPoints - 1;

    // prevEditIdx ~ changedIndex ~ nextEditIdx 구간만 cubic spline 보간 (semitones 기반)
    if (nextEditIdx > prevEditIdx + 1) {
      // 주파수를 semitones로 변환 (청각적으로 자연스러운 보간을 위해)
      float prevFreq = pitchData[prevEditIdx].frequency;
      float nextFreq = pitchData[nextEditIdx].frequency;
      float prevSemitones = 12.0f * std::log2(prevFreq / 440.0f);
      float nextSemitones = 12.0f * std::log2(nextFreq / 440.0f);

      // Tangent 추정 (semitones 기반)
      float m0 = 0.0f, m1 = 0.0f;
      if (prevEditIdx > 0) {
        float beforeFreq = pitchData[prevEditIdx - 1].frequency;
        float beforeSemitones = 12.0f * std::log2(beforeFreq / 440.0f);
        m0 = (nextSemitones - beforeSemitones) / 2.0f;
      }
      if (nextEditIdx < numPoints - 1) {
        float afterFreq = pitchData[nextEditIdx + 1].frequency;
        float afterSemitones = 12.0f * std::log2(afterFreq / 440.0f);
        m1 = (afterSemitones - prevSemitones) / 2.0f;
      }

      // Cubic hermite spline 보간
      for (int i = prevEditIdx + 1; i < nextEditIdx; ++i) {
        if (i == changedIndex) continue;  // 변경된 포인트는 건드리지 않음

        float t = (pitchData[i].time - pitchData[prevEditIdx].time) /
                  (pitchData[nextEditIdx].time - pitchData[prevEditIdx].time);

        // Hermite basis functions
        float h00 = 2*t*t*t - 3*t*t + 1;
        float h10 = t*t*t - 2*t*t + t;
        float h01 = -2*t*t*t + 3*t*t;
        float h11 = t*t*t - t*t;

        // Semitones 보간
        float interpolatedSemitones = h00 * prevSemitones + h10 * m0 + h01 * nextSemitones + h11 * m1;

        // Semitones를 주파수로 다시 변환
        float interpolatedFreq = 440.0f * std::pow(2.0f, interpolatedSemitones / 12.0f);
        pitchData[i].frequency = interpolatedFreq;
      }
    }
  }

  // 5. 결과 반환
  emscripten::val result = emscripten::val::object();

  // 수정된 pitchData
  emscripten::val pitchDataResult = emscripten::val::array();
  for (const auto& p : pitchData) {
    emscripten::val obj = emscripten::val::object();
    obj.set("time", p.time);
    obj.set("frequency", p.frequency);
    obj.set("confidence", p.confidence);
    pitchDataResult.call<void>("push", obj);
  }

  // 편집 인덱스는 그대로 반환 (변경 없음)
  emscripten::val editIndicesResult = emscripten::val::array();
  for (int idx : allEditIndices) {
    editIndicesResult.call<void>("push", idx);
  }

  result.set("pitchData", pitchDataResult);
  result.set("editIndices", editIndicesResult);

  return result;
}

/**
 * 편집 포인트 매니저: 초기화 (첫 렌더링)
 * Pitch 데이터에서 편집 가능한 포인트를 찾아서 초기화
 *
 * @return 편집 가능한 포인트 배열 [{time, frequency, semitones}]
 */
emscripten::val editPointInitialize(
    emscripten::val pitchDataJS,
    float minDistance = 0.1f,
    float confidenceThreshold = 0.3f,
    int maxPoints = 50
) {
  init();

  // JS 배열 파싱
  int numPoints = pitchDataJS["length"].as<int>();
  std::vector<PitchPoint> pitchData;
  for (int i = 0; i < numPoints; ++i) {
    emscripten::val point = pitchDataJS[i];
    PitchPoint p;
    p.time = point["time"].as<float>();
    p.frequency = point["frequency"].as<float>();
    p.confidence = point["confidence"].as<float>();
    pitchData.push_back(p);
  }

  // 편집 가능한 포인트 찾기
  std::vector<PitchPoint> peaks = g_editPointManager->findEditablePoints(
    pitchData, minDistance, confidenceThreshold, maxPoints
  );

  // EditPointManager에 초기화 (semitones=0, outlier correction 없음)
  g_editPointManager->reset();
  for (const auto& peak : peaks) {
    g_editPointManager->updateEditPoint(peak.time, 0.0f, 0.0f, 48000, 0.0f);
  }

  // JavaScript 배열로 변환 (렌더링용)
  emscripten::val result = emscripten::val::array();
  for (const auto& peak : peaks) {
    emscripten::val obj = emscripten::val::object();
    obj.set("time", peak.time);
    obj.set("frequency", peak.frequency);
    obj.set("semitones", 0.0f);  // 초기값
    result.call<void>("push", obj);
  }

  return result;
}

/**
 * 편집 포인트 매니저: 현재 모든 편집 포인트 가져오기
 *
 * @return 편집 포인트 배열 [{time, semitones}]
 */
emscripten::val editPointGetAll() {
  init();

  const auto& editPoints = g_editPointManager->getAllEditPoints();

  emscripten::val result = emscripten::val::array();
  for (const auto& [time, semitones] : editPoints) {
    emscripten::val obj = emscripten::val::object();
    obj.set("time", time);
    obj.set("semitones", semitones);
    result.call<void>("push", obj);
  }

  return result;
}

/**
 * 편집 포인트 매니저: 포인트 업데이트
 *
 * @param time 편집 시간
 * @param semitones 변경할 semitones
 * @param totalDuration 오디오 전체 길이
 * @param sampleRate 샘플레이트
 * @return 보정된 편집 포인트 배열 [{time, semitones}]
 */
emscripten::val editPointUpdate(
    float time,
    float semitones,
    float totalDuration,
    int sampleRate,
    float gradientThreshold = 3.0f,
    float frameInterval = 0.02f
) {
  init();

  // 1. 편집 포인트 업데이트 (부분 outlier correction 포함)
  g_editPointManager->updateEditPoint(
    time, semitones, totalDuration, sampleRate, gradientThreshold
  );

  // 2. 보정된 편집 포인트들 직접 반환 (SplineInterpolator 거치지 않음)
  const auto& correctedPoints = g_editPointManager->getCorrectedEditPoints();

  emscripten::val result = emscripten::val::array();
  for (const auto& [t, s] : correctedPoints) {
    emscripten::val obj = emscripten::val::object();
    obj.set("time", t);
    obj.set("semitones", s);
    obj.set("isOutlier", false);  // 이미 보정됨
    result.call<void>("push", obj);
  }

  return result;
}

/**
 * 편집 포인트 매니저: 보간된 전체 프레임 가져오기 (선 그리기용)
 */
emscripten::val editPointGetInterpolated(
    float totalDuration,
    int sampleRate,
    float gradientThreshold = 3.0f,
    float frameInterval = 0.02f
) {
  init();

  std::vector<FrameData> interpolatedFrames = g_editPointManager->getInterpolatedFrames(
    totalDuration, sampleRate, gradientThreshold, frameInterval
  );

  // JavaScript 배열로 변환
  emscripten::val result = emscripten::val::array();
  for (const auto& frame : interpolatedFrames) {
    emscripten::val obj = emscripten::val::object();
    obj.set("time", frame.time);
    obj.set("pitchSemitones", frame.pitchSemitones);
    obj.set("isEdited", frame.isEdited);
    obj.set("isOutlier", frame.isOutlier);
    obj.set("isInterpolated", frame.isInterpolated);
    obj.set("editTime", frame.editTime);
    result.call<void>("push", obj);
  }

  return result;
}

/**
 * 편집 포인트 매니저: 초기화
 */
void editPointReset() {
  init();
  g_editPointManager->reset();
}

/**
 * 새로운 파이프라인 아키텍처: 전처리 + 보간 (그래프 표시용)
 *
 * JavaScript 편집 포인트를 받아서:
 * 1. Outlier Correction (gradient 기반)
 * 2. Cubic Spline Interpolation
 *
 * 반환: 보간된 FrameData 배열 (그래프 표시용)
 */
emscripten::val preprocessAndInterpolate(
    float totalDuration,
    int sampleRate,
    emscripten::val editPointsJS,
    float gradientThreshold,
    float frameInterval
) {
  init();

  // JS Array에서 Edit Points 파싱
  std::vector<FrameData> editPoints;
  int numPoints = editPointsJS["length"].as<int>();

  for (int i = 0; i < numPoints; ++i) {
    emscripten::val point = editPointsJS[i];

    FrameData frame;
    frame.time = point["time"].as<float>();
    frame.pitchSemitones = point["semitones"].as<float>();
    frame.isEdited = true;  // 모든 편집 포인트는 isEdited = true
    frame.isOutlier = false;
    frame.isInterpolated = false;

    editPoints.push_back(frame);
  }

  // Pipeline 생성 (전처리용)
  PitchFirstPipeline pipeline(gradientThreshold, frameInterval);

  // 전처리만 실행 (outlier correction + spline interpolation)
  std::vector<FrameData> interpolatedFrames = pipeline.preprocessOnly(
      editPoints, totalDuration, sampleRate
  );

  // JavaScript 배열로 변환
  emscripten::val result = emscripten::val::array();

  for (const auto& frame : interpolatedFrames) {
    emscripten::val obj = emscripten::val::object();
    obj.set("time", frame.time);
    obj.set("pitchSemitones", frame.pitchSemitones);
    obj.set("isEdited", frame.isEdited);
    obj.set("isOutlier", frame.isOutlier);
    obj.set("isInterpolated", frame.isInterpolated);
    obj.set("editTime", frame.editTime);  // 원본 편집 시간 (pitchEdits 키)
    result.call<void>("push", obj);
  }

  return result;
}

/**
 * 새로운 파이프라인 아키텍처: 전체 처리 (오디오 생성용)
 *
 * 전처리된 FrameData와 원본 오디오를 받아서:
 * 1. Pitch 처리 (선택된 알고리즘)
 * 2. Duration 처리 (옵션, 선택된 알고리즘)
 * 3. Reconstructor (FrameData → AudioBuffer)
 *
 * 반환: 처리된 오디오 (Float32Array)
 */
emscripten::val processAudioWithPipeline(
    uintptr_t dataPtr,
    int length,
    int sampleRate,
    emscripten::val interpolatedFramesJS,
    const std::string& pitchAlgorithm,
    const std::string& durationAlgorithm,
    bool previewMode,
    float gradientThreshold,
    float frameInterval
) {
  init();

  // 1. 오디오 데이터 변환
  float* audioData = reinterpret_cast<float*>(dataPtr);
  std::vector<float> samples(audioData, audioData + length);

  // 2. JS Array에서 Interpolated Frames 파싱
  std::vector<FrameData> frames;
  int numFrames = interpolatedFramesJS["length"].as<int>();

  for (int i = 0; i < numFrames; ++i) {
    emscripten::val frameJS = interpolatedFramesJS[i];

    FrameData frame;
    frame.time = frameJS["time"].as<float>();
    frame.pitchSemitones = frameJS["pitchSemitones"].as<float>();
    frame.isEdited = frameJS["isEdited"].as<bool>();
    frame.isOutlier = frameJS["isOutlier"].as<bool>();
    frame.isInterpolated = frameJS["isInterpolated"].as<bool>();
    frame.editTime = frameJS["editTime"].as<float>();

    frames.push_back(frame);
  }

  // 3. Pitch Processor 생성
  std::unique_ptr<IPitchProcessor> pitchProcessor;

  if (pitchAlgorithm == "psola") {
    pitchProcessor.reset(new PSOLAPitchProcessor(2048, 512));
  } else if (pitchAlgorithm == "phase-vocoder") {
    pitchProcessor.reset(new PhaseVocoderPitchProcessor(2048, 512, true));
  } else if (pitchAlgorithm == "soundtouch") {
    pitchProcessor.reset(new SoundTouchPitchProcessor(2048, 512));
  } else if (pitchAlgorithm == "rubberband") {
    pitchProcessor.reset(new RubberBandPitchProcessor(2048, 512));
  } else {
    // 기본값: Phase Vocoder
    pitchProcessor.reset(new PhaseVocoderPitchProcessor(2048, 512, true));
  }

  // 4. Duration Processor 생성
  std::unique_ptr<IDurationProcessor> durationProcessor;

  if (durationAlgorithm == "wsola") {
    durationProcessor.reset(new WSOLADurationProcessor(1024, 512));
  } else if (durationAlgorithm == "soundtouch") {
    durationProcessor.reset(new SoundTouchDurationProcessor());
  } else if (durationAlgorithm == "rubberband") {
    durationProcessor.reset(new RubberBandDurationProcessor(2048, 512));
  } else if (durationAlgorithm == "none" || durationAlgorithm.empty()) {
    durationProcessor = nullptr;
  } else {
    // 기본값: nullptr (duration 처리 안 함)
    durationProcessor = nullptr;
  }

  // 5. Pipeline 생성 및 실행
  AudioBuffer result;

  if (pitchAlgorithm == "hybrid") {
    // Hybrid Pipeline (Preview/Final 모드)
    HybridPipeline pipeline(previewMode, gradientThreshold, frameInterval);
    result = pipeline.execute(samples, frames, sampleRate, nullptr, durationProcessor.get());
  } else {
    // Standard Pitch-First Pipeline
    PitchFirstPipeline pipeline(gradientThreshold, frameInterval);
    result = pipeline.execute(samples, frames, sampleRate, pitchProcessor.get(), durationProcessor.get());
  }

  // 6. Float32Array로 변환하여 반환
  const std::vector<float>& outputData = result.getData();
  emscripten::val outputArray = emscripten::val::global("Float32Array").new_(outputData.size());

  for (size_t i = 0; i < outputData.size(); ++i) {
    outputArray.set(i, outputData[i]);
  }

  return outputArray;
}

EMSCRIPTEN_BINDINGS (audio_module) {
  // 기본 함수
  function("init", &init);
  function("startRecording", &startRecording);
  function("stopRecording", &stopRecording);
  function("addAudioData", &addAudioData);
  function("getRecordedAudioAsWav", &getRecordedAudioAsWav);

  // 분석 함수
  function("analyzePitch", &analyzePitch);
  function("analyzeDuration", &analyzeDuration);
  function("analyzePower", &analyzePower);
  function("analyzeQuality", &analyzeQuality);

  // 인터랙티브 편집용 함수
  function("getFrameDataArray", &getFrameDataArray);

  // applyEdits 함수들 제거됨 - 새 Pipeline 아키텍처 사용

  // 새로운 pitchData 기반 API
  function("generateEditPoints", &generateEditPoints);
  function("processPitchData", &processPitchData);

  // 편집 포인트 매니저 (레거시, processPitchData 사용 권장)
  function("editPointInitialize", &editPointInitialize);
  function("editPointUpdate", &editPointUpdate);
  function("editPointGetAll", &editPointGetAll);
  function("editPointGetInterpolated", &editPointGetInterpolated);
  function("editPointReset", &editPointReset);

  // 새로운 파이프라인 아키텍처 (레거시, 편집 포인트 매니저 사용 권장)
  function("preprocessAndInterpolate", &preprocessAndInterpolate);
  function("processAudioWithPipeline", &processAudioWithPipeline);

  // 효과 함수
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
