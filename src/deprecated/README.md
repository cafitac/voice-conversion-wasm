# Deprecated Files

이 폴더에는 더 이상 사용되지 않는 파일들이 보관됩니다.

## 목적
- 참고용으로 보관 (완전히 제거하기 전)
- 어떤 소스 코드에서도 이 폴더의 파일을 참조하지 않음
- 필요시 복구 가능

## Deprecated 파일 목록

### effects/ (Strategy 패턴 - 2025-11-12 제거됨)

**Strategy 인터페이스** (2개):
- `IPitchShiftStrategy.h` - Pitch Shift Strategy 인터페이스
- `ITimeStretchStrategy.h` - Time Stretch Strategy 인터페이스

**Pitch Shift Strategies** (10개):
- `FastPitchShiftStrategy.{h,cpp}` - 단순 리샘플링
- `HighQualityPitchShiftStrategy.{h,cpp}` - Phase Vocoder 기반
- `ExternalPitchShiftStrategy.{h,cpp}` - SoundTouch 사용
- `RubberBandPitchShiftStrategy.{h,cpp}` - RubberBand 사용
- `PSOLAPitchShiftStrategy.{h,cpp}` - PSOLA 알고리즘

**Time Stretch Strategies** (10개):
- `FastTimeStretchStrategy.{h,cpp}` - 프레임 반복/스킵
- `HighQualityTimeStretchStrategy.{h,cpp}` - WSOLA
- `ExternalTimeStretchStrategy.{h,cpp}` - SoundTouch 사용
- `PhaseVocoderTimeStretchStrategy.{h,cpp}` - Phase Vocoder
- `RubberBandTimeStretchStrategy.{h,cpp}` - RubberBand 사용

**Strategy 의존 클래스** (8개):
- `FramePitchModifier.{h,cpp}` - 프레임별 Pitch 수정
- `TimeScaleModifier.{h,cpp}` - 시간 스케일 수정
- `HighQualityPerFrameEditor.{h,cpp}` - 고품질 프레임 편집기
- `ExternalPerFrameEditor.{h,cpp}` - 외부 라이브러리 편집기

**사용하지 않는 레거시 클래스** (4개, 2025-11-12 추가):
- `PitchShifter.{h,cpp}` - 단순 리샘플링 구현 (사용 안 함)
- `TimeStretcher.{h,cpp}` - Time stretching 구현 (사용 안 함)

**총 34개 파일**

### visualization/ (Canvas 기반 시각화 - 2025-11-12 제거됨)

**Visualization 클래스** (4개):
- `CanvasRenderer.{h,cpp}` - Canvas API 기반 그래프 렌더링
- `TrimController.{h,cpp}` - Canvas 기반 Trim UI 컨트롤러

**제거 이유**:
- 모든 그래프 렌더링이 D3.js로 전환됨
- Canvas 기반 렌더링 코드는 더 이상 사용되지 않음
- `analysisCanvas` 엘리먼트가 HTML에서 제거됨
- `analyzeVoice()` 함수가 JavaScript에서 제거됨
- 11개 EMSCRIPTEN_BINDINGS 함수 제거됨:
  - `drawCombinedAnalysis()`
  - `drawTrimHandles()`
  - `enableTrimMode()`, `disableTrimMode()`
  - `trimMouseDown()`, `trimMouseMove()`, `trimMouseUp()`
  - `getTrimStart()`, `getTrimEnd()`
  - `isTrimDragging()`, `resetTrimHandles()`

**대체 기술**: D3.js를 사용한 SVG 기반 인터랙티브 차트

**총 38개 파일** (34개 + 4개 visualization)

## 대체 아키텍처

### 기존: Strategy 패턴 (제거됨)
```
User Input → Strategy Selection → Global Strategy → Processing
```

### 현재: Pipeline 아키텍처
```
User Input → Edit Points
            ↓
       [Preprocessors]
         - OutlierCorrector
         - SplineInterpolator
            ↓
       Interpolated Frames
            ↓
        [Pipeline]
            ↓
       [Processors]
         - Pitch Processor
         - Duration Processor
            ↓
       [Reconstructor]
            ↓
         AudioBuffer
```

## 마이그레이션 완료 내역

### ✅ Phase 1: main.cpp 함수 제거 (2025-11-12 완료)
다음 main.cpp 함수들이 **완전히 제거됨**:
- `setPitchShiftQuality()` / `getPitchShiftQuality()`
- `setTimeStretchQuality()` / `getTimeStretchQuality()`
- `applyPitchShift()`
- `applyVariablePitchShift()`
- `applyTimeStretch()`
- `applyEditsHighQuality()`
- `applyEditsExternal()`
- `applyEditsHighQualityWithKeyPoints()`
- `applyEditsExternalWithKeyPoints()`

**대체 함수**:
- `preprocessAndInterpolate()` - 전처리 및 보간
- `processAudioWithPipeline()` - 통합 오디오 처리

### ✅ Phase 2: JavaScript 마이그레이션 (2025-11-12 완료)
JavaScript 코드가 완전히 새로운 Pipeline 아키텍처로 마이그레이션됨:
- `ui-controller.js`의 `applyPitchShift()` → Pipeline 사용
- `ui-controller.js`의 `applyTimeStretch()` → Pipeline 사용
- Deprecated 헬퍼 함수 제거:
  - `applyInterpolatedPitchShift()`
  - `applyDurationEdits()`
  - `applyPitchShiftWithAlgorithm()`
  - `applyTimeStretchWithAlgorithm()`

### ✅ Phase 3: Strategy 클래스 제거 (2025-11-12 완료)
모든 Strategy 클래스와 의존 클래스가 `src/deprecated/effects/`로 이동됨:
- Strategy 패턴 관련: 30개 파일
- 사용하지 않는 레거시: 4개 파일
- **총 34개 파일** 이동 완료
- `build.sh`에서 제거됨
- 더 이상 컴파일되지 않음

### ✅ Phase 4: Visualization 클래스 제거 (2025-11-12 완료)
Canvas 기반 시각화 코드가 `src/deprecated/visualization/`로 이동됨:
- CanvasRenderer + TrimController: 4개 파일
- D3.js로 완전히 대체됨
- `build.sh`에서 제거됨
- main.cpp에서 11개 함수 바인딩 제거됨
- ui-controller.js에서 `analyzeVoice()` 제거됨
- **총 38개 파일** deprecated (34개 + 4개)

## 새로운 아키텍처 위치

**Preprocessors**: `src/preprocessor/`
- `OutlierCorrector.{h,cpp}` - Gradient 기반 outlier 감지/보정
- `SplineInterpolator.{h,cpp}` - Cubic Spline 보간

**Processors**: `src/processor/`
- **Pitch**: `src/processor/pitch/`
  - `PSOLAPitchProcessor.{h,cpp}`
  - `PhaseVocoderPitchProcessor.{h,cpp}`
  - `SoundTouchPitchProcessor.{h,cpp}`
  - `RubberBandPitchProcessor.{h,cpp}`
- **Duration**: `src/processor/duration/`
  - `WSOLADurationProcessor.{h,cpp}`
  - `SoundTouchDurationProcessor.{h,cpp}`
  - `RubberBandDurationProcessor.{h,cpp}`

**Pipelines**: `src/pipeline/`
- `PitchFirstPipeline.{h,cpp}` - Pitch → Duration 순서
- `HybridPipeline.{h,cpp}` - Preview/Final 모드

**Algorithms**: `src/algorithm/`
- `src/algorithm/pitch/` - Low-level Pitch 알고리즘
- `src/algorithm/duration/` - Low-level Duration 알고리즘

## 주의사항

⚠️ **이 폴더의 파일들은 빌드 시스템(build.sh)에 포함되지 않습니다.**

⚠️ **어떤 소스 코드에서도 이 폴더를 #include 하지 마세요.**

⚠️ **참고 목적으로만 유지됩니다. 향후 완전히 삭제될 수 있습니다.**

## 문서

- [새로운 아키텍처 가이드](../../docs/NEW_ARCHITECTURE.md)
- [구현 완료 보고서](../../docs/IMPLEMENTATION_SUMMARY.md)
- [Quick Start 가이드](../../docs/QUICK_START.md)

---

**Last Updated**: 2025-11-12
**Status**: ✅ 리팩토링 완료 (Phase 1-4)
- Phase 1-3: Strategy 패턴 제거 (34개 파일)
- Phase 4: Canvas 시각화 제거 (4개 파일)
- **총 38개 파일** deprecated
- **D3.js 기반 SVG 차트로 완전 전환**
