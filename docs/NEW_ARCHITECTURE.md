# 새로운 Variable Pitch/Duration 아키텍처

## 개요

이 문서는 Variable Pitch Shift 시스템의 새로운 아키텍처를 설명합니다. 기존의 Strategy 패턴 기반 시스템을 대체하는 Preprocessor → Processor → Pipeline 아키텍처입니다.

## 아키텍처 개요

```
Edit Points (User Input)
         ↓
  [Preprocessors]
    - OutlierCorrector (Gradient-based)
    - SplineInterpolator (Cubic Spline)
         ↓
  Interpolated Frames
         ↓
    [Pipeline]
         ↓
  [Pitch Processor] → [Duration Processor] → [Reconstructor]
         ↓
   AudioBuffer (Output)
```

## 주요 컴포넌트

### 1. FrameData 구조체

모든 처리의 기본 단위입니다.

```cpp
struct FrameData {
    float time;                   // 프레임 시간 (초)
    std::vector<float> samples;   // 오디오 샘플
    float rms;                    // RMS 값
    bool isVoice;                 // 음성 여부

    // Variable Pitch/Duration 관련
    float pitchSemitones;         // Pitch shift 양 (semitones)
    float durationRatio;          // Duration ratio (1.0 = 원본 속도)
    float originalPitchHz;        // 원본 pitch (Hz)

    // 메타데이터
    bool isEdited;                // 사용자 편집 포인트 여부
    bool isOutlier;               // Outlier로 감지되어 보정됨
    bool isInterpolated;          // Spline 보간으로 생성됨
};
```

### 2. Preprocessors (전처리기)

#### 2.1 OutlierCorrector

**목적**: Gradient 기반으로 outlier를 감지하고 보정합니다.

**특징**:
- 절대값이 아닌 **프레임 간 변화율(gradient)** 기반 감지
- 인접 프레임과의 연속성을 고려
- 가중 평균(weighted average)으로 보정
- Multi-pass iteration 지원

**사용법**:
```cpp
OutlierCorrector corrector(
    3.0f,  // gradientThreshold (기본 3.0 semitones)
    2,     // windowSize (좌우 2개 프레임 참고)
    3      // maxIterations (최대 3회 반복)
);

std::vector<FrameData> corrected = corrector.process(frames);
```

**주요 메서드**:
- `process()`: Outlier correction 실행
- `setGradientThreshold()`: Threshold 조정
- `isOutlier()`: 특정 프레임이 outlier인지 확인

#### 2.2 SplineInterpolator

**목적**: 편집 포인트 사이를 Cubic Spline으로 부드럽게 보간합니다.

**특징**:
- Natural Cubic Spline (C2 연속성)
- Thomas Algorithm 사용 (O(n) 복잡도)
- 편집 포인트 사이만 보간 (편집 포인트는 그대로 유지)

**사용법**:
```cpp
SplineInterpolator interpolator(
    0.02f,  // frameInterval (20ms 간격)
    48000   // sampleRate
);

std::vector<FrameData> interpolated = interpolator.process(editedFrames);
```

### 3. Processors (프로세서)

#### 3.1 Pitch Processors

모든 Pitch Processor는 `IPitchProcessor` 인터페이스를 구현합니다.

**사용 가능한 Pitch Processors**:

| Processor | Algorithm | 특징 | 적합한 용도 |
|-----------|-----------|------|------------|
| PSOLAPitchProcessor | PSOLA | 네이티브 variable pitch, 빠름 | 음성, 실시간 미리듣기 |
| PhaseVocoderPitchProcessor | Phase Vocoder | 고품질, Formant 보존 | 음성, 최종 출력 |
| SoundTouchPitchProcessor | SoundTouch | 안정적, 프로덕션 검증됨 | 일반 용도 |
| RubberBandPitchProcessor | RubberBand | 최고 품질, Formant 보존 | 음악, 최종 출력 |

**공통 인터페이스**:
```cpp
class IPitchProcessor {
public:
    virtual std::vector<FrameData> process(
        const std::vector<FrameData>& frames,
        int sampleRate
    ) = 0;

    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;
};
```

**사용 예시**:
```cpp
// 1. Processor 생성
auto processor = std::make_unique<PSOLAPitchProcessor>(2048, 512);

// 2. 처리
std::vector<FrameData> result = processor->process(frames, sampleRate);
```

#### 3.2 Duration Processors

모든 Duration Processor는 `IDurationProcessor` 인터페이스를 구현합니다.

**사용 가능한 Duration Processors**:

| Processor | Algorithm | 특징 | 처리 방식 |
|-----------|-----------|------|----------|
| WSOLADurationProcessor | WSOLA | 빠른 처리 | Frame-by-frame (200ms) |
| SoundTouchDurationProcessor | SoundTouch | 안정적, 권장 | Frame-by-frame (200ms) |
| RubberBandDurationProcessor | RubberBand | 최고 품질 | Frame-by-frame (200ms) |

**Frame-by-frame 처리 방식**:
- 200ms 프레임으로 분할
- 50% overlap (100ms)
- 각 프레임 중앙 시간의 durationRatio 사용
- Crossfade로 부드러운 연결

**사용 예시**:
```cpp
// 1. Processor 생성
auto processor = std::make_unique<SoundTouchDurationProcessor>();

// 2. 처리 (각 FrameData의 durationRatio 사용)
std::vector<FrameData> result = processor->process(frames, sampleRate);
```

### 4. Pipelines

Pipeline은 전처리부터 후처리까지의 전체 흐름을 관리합니다.

#### 4.1 PitchFirstPipeline

**처리 순서**: Pitch → Duration

```
Input Audio + Interpolated Frames
         ↓
[1] populateAudioSamples (오디오를 FrameData에 채움)
         ↓
[2] Pitch Processing (IPitchProcessor)
         ↓
[3] Duration Processing (IDurationProcessor, 옵션)
         ↓
[4] Reconstruction (FrameReconstructor)
         ↓
    AudioBuffer Output
```

**사용법**:
```cpp
// 1. Pipeline 생성
PitchFirstPipeline pipeline(
    3.0f,   // gradientThreshold
    0.02f   // frameInterval
);

// 2. 전처리만 실행 (그래프 표시용)
std::vector<FrameData> interpolated = pipeline.preprocessOnly(
    editPoints, totalDuration, sampleRate
);

// 3. 전체 처리 실행 (오디오 생성용)
AudioBuffer result = pipeline.execute(
    audioData,           // 원본 오디오
    interpolated,        // 전처리된 프레임들
    sampleRate,
    pitchProcessor,      // Pitch 알고리즘
    durationProcessor    // Duration 알고리즘 (nullptr 가능)
);
```

#### 4.2 HybridPipeline

**특징**: Preview/Final 모드 전환 지원

- **Preview 모드**: PSOLA (빠른 처리, 실시간 미리듣기)
- **Final 모드**: Phase Vocoder (고품질, 최종 출력)

**사용법**:
```cpp
HybridPipeline pipeline(
    true,   // previewMode (true = PSOLA, false = Phase Vocoder)
    3.0f,   // gradientThreshold
    0.02f   // frameInterval
);

AudioBuffer result = pipeline.execute(
    audioData, interpolated, sampleRate,
    nullptr,           // pitchProcessor (자동 선택됨)
    durationProcessor
);
```

## JavaScript 인터페이스

### 1. preprocessAndInterpolate (그래프 표시용)

편집 포인트를 받아서 전처리 + 보간만 수행합니다.

```javascript
const interpolatedFrames = Module.preprocessAndInterpolate(
    totalDuration,      // 전체 오디오 길이 (초)
    sampleRate,         // 샘플 레이트
    editPoints,         // [{time, semitones}, ...]
    3.0,               // gradientThreshold
    0.02               // frameInterval
);

// interpolatedFrames: [{
//     time,
//     pitchSemitones,
//     isEdited,        // 사용자 편집 포인트
//     isOutlier,       // Outlier로 감지됨
//     isInterpolated   // Spline 보간으로 생성됨
// }, ...]
```

**반환값 활용**:
- 그래프에 표시 (편집/보간/outlier 구분)
- 다음 단계의 `processAudioWithPipeline()`에 전달

### 2. processAudioWithPipeline (오디오 생성용)

전처리된 프레임과 원본 오디오를 받아서 처리합니다.

```javascript
const dataPtr = Module._malloc(audio.length * 4);
Module.HEAPF32.set(audio, dataPtr / 4);

const resultView = Module.processAudioWithPipeline(
    dataPtr,                    // 원본 오디오 포인터
    audio.length,               // 샘플 수
    sampleRate,                 // 샘플 레이트
    interpolatedFrames,         // 전처리된 프레임들
    "phase-vocoder",           // Pitch 알고리즘
    "soundtouch",              // Duration 알고리즘 (또는 "none")
    false,                     // Preview 모드 (hybrid일 때만 사용)
    3.0,                       // gradientThreshold
    0.02                       // frameInterval
);

// 결과를 Float32Array로 복사
const result = new Float32Array(resultView.length);
for (let i = 0; i < resultView.length; i++) {
    result[i] = resultView[i];
}

Module._free(dataPtr);
```

**지원 알고리즘**:

Pitch 알고리즘:
- `"psola"` - 빠른 처리
- `"phase-vocoder"` - 고품질
- `"soundtouch"` - 안정적
- `"rubberband"` - 최고 품질
- `"hybrid"` - 자동 전환

Duration 알고리즘:
- `"none"` - Duration 처리 안 함
- `"wsola"` - 빠른 처리
- `"soundtouch"` - 안정적
- `"rubberband"` - 최고 품질

## 전체 워크플로우 예시

### JavaScript에서 Variable Pitch Shift 적용

```javascript
// 1단계: 사용자 편집 포인트 수집
const editPoints = [
    { time: 0.0, semitones: 0.0 },
    { time: 1.0, semitones: 2.0 },
    { time: 2.0, semitones: -1.0 },
    { time: 3.0, semitones: 0.0 }
];

// 2단계: 전처리 + 보간
const interpolatedFrames = Module.preprocessAndInterpolate(
    3.0,           // totalDuration
    48000,         // sampleRate
    editPoints,
    3.0,           // gradientThreshold
    0.02           // frameInterval
);

// 3단계: 그래프에 표시
for (const frame of interpolatedFrames) {
    if (frame.isOutlier) {
        // Outlier 표시 (빨간색)
        drawPoint(frame.time, frame.pitchSemitones, 'red');
    } else if (frame.isEdited) {
        // 편집 포인트 표시 (파란색)
        drawPoint(frame.time, frame.pitchSemitones, 'blue');
    } else {
        // 보간 포인트 표시 (회색)
        drawPoint(frame.time, frame.pitchSemitones, 'gray');
    }
}

// 4단계: 오디오 처리
const dataPtr = Module._malloc(audio.length * 4);
Module.HEAPF32.set(audio, dataPtr / 4);

const resultView = Module.processAudioWithPipeline(
    dataPtr,
    audio.length,
    48000,
    interpolatedFrames,
    "phase-vocoder",  // 고품질 Pitch
    "soundtouch",     // 안정적 Duration
    false,
    3.0,
    0.02
);

const result = new Float32Array(resultView.length);
for (let i = 0; i < resultView.length; i++) {
    result[i] = resultView[i];
}

Module._free(dataPtr);

// 5단계: 결과 재생
playAudio(result, 48000);
```

## 성능 특성

### Pitch Processors 비교

| Algorithm | 처리 속도 | 품질 | Variable Pitch | Formant 보존 |
|-----------|----------|------|---------------|-------------|
| PSOLA | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | Native | ✗ |
| Phase Vocoder | ⭐⭐⭐ | ⭐⭐⭐⭐ | Frame-by-frame | ✓ |
| SoundTouch | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | Frame-by-frame | ✗ |
| RubberBand | ⭐⭐ | ⭐⭐⭐⭐⭐ | Frame-by-frame | ✓ |

### Duration Processors 비교

| Algorithm | 처리 속도 | 품질 | Frame 크기 |
|-----------|----------|------|----------|
| WSOLA | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | 200ms |
| SoundTouch | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 200ms |
| RubberBand | ⭐⭐ | ⭐⭐⭐⭐⭐ | 200ms |

### 권장 조합

**음성 실시간 미리듣기**:
- Pitch: PSOLA
- Duration: WSOLA
- 예상 처리 시간: 1-2초 (3분 오디오 기준)

**음성 최종 출력**:
- Pitch: Phase Vocoder
- Duration: SoundTouch
- 예상 처리 시간: 5-10초 (3분 오디오 기준)

**음악 최고 품질**:
- Pitch: RubberBand
- Duration: RubberBand
- 예상 처리 시간: 15-30초 (3분 오디오 기준)

## 마이그레이션 가이드

### ⚠️ 기존 Strategy 패턴 코드 (제거됨)

아래 함수들은 **완전히 제거**되었습니다:

```javascript
// ❌ REMOVED: 더 이상 사용 불가
Module.setPitchShiftQuality('high');
Module.setTimeStretchQuality('high');
Module.applyPitchShift(dataPtr, length, sampleRate, 2.0);
Module.applyVariablePitchShift(dataPtr, length, sampleRate, editPoints);
Module.applyTimeStretch(dataPtr, length, sampleRate, 1.5);
Module.applyEditsHighQuality(...);
Module.applyEditsExternal(...);
```

### ✅ 새로운 Pipeline 아키텍처

모든 오디오 처리는 이제 다음 두 함수를 사용합니다:

**1단계: 편집 포인트 전처리 및 보간**
```javascript
const editPoints = [
    { time: 0, semitones: 2.0 },
    { time: duration, semitones: 2.0 }
];

const interpolatedFrames = Module.preprocessAndInterpolate(
    duration,           // 오디오 총 길이 (초)
    sampleRate,         // 샘플레이트
    editPoints,         // 편집 포인트 배열
    3.0,               // gradientThreshold
    0.02               // frameInterval
);
```

**2단계: 오디오 처리**
```javascript
const result = Module.processAudioWithPipeline(
    dataPtr,            // 오디오 데이터 포인터
    length,             // 샘플 수
    sampleRate,         // 샘플레이트
    interpolatedFrames, // 1단계 결과
    "phase-vocoder",    // Pitch 알고리즘
    "soundtouch",       // Duration 알고리즘 (또는 "none")
    false,              // previewMode
    3.0,               // gradientThreshold
    0.02               // frameInterval
);
```

## 디버깅

### 로그 활성화

JavaScript 콘솔에서 처리 과정을 확인할 수 있습니다:

```javascript
console.log(`✓ Interpolated ${interpolatedFrames.length} frames`);
console.log(`✓ Outliers corrected: ${
    interpolatedFrames.filter(f => f.isOutlier).length
}`);
console.log(`✓ Processing with: pitch=${pitchAlgo}, duration=${durationAlgo}`);
```

### 일반적인 문제

**1. Outlier가 너무 많이 감지됨**
- `gradientThreshold`를 높임 (예: 3.0 → 5.0)
- 편집 포인트를 더 부드럽게 배치

**2. 보간이 부자연스러움**
- `frameInterval`을 줄임 (예: 0.02 → 0.01)
- Cubic spline은 C2 연속성을 보장하지만, 너무 급격한 변화는 부자연스러울 수 있음

**3. 처리가 너무 느림**
- 빠른 알고리즘으로 변경 (PSOLA, WSOLA)
- Preview 모드 사용 (Hybrid Pipeline)

## 추가 정보

### 파일 위치

- **Preprocessors**: `src/preprocessor/`
- **Processors**: `src/processor/pitch/`, `src/processor/duration/`
- **Algorithms**: `src/algorithm/pitch/`, `src/algorithm/duration/`
- **Pipelines**: `src/pipeline/`
- **Deprecated**: `src/deprecated/`

### 관련 문서

- [Deprecated README](../src/deprecated/README.md) - Deprecation 계획
- [Build Instructions](../README.md) - 빌드 방법

### 라이선스

- **PSOLA, Phase Vocoder, WSOLA**: 프로젝트 라이선스
- **SoundTouch**: LGPL 2.1
- **RubberBand**: GPL 2.0 (상업적 사용 시 별도 라이선스 필요)
