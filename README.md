# 🎵 실시간 오디오 처리 시스템

WebAssembly 기반의 실시간 오디오 처리 애플리케이션입니다. 피치 변조(Pitch Shift), 시간 늘이기(Time Stretch), 다양한 오디오 필터를 웹 브라우저에서 직접 처리할 수 있습니다.

## ✨ 주요 기능

### 🎼 오디오 처리 기능
- **피치 변조 (Pitch Shift)**: 음높이 변경 (-12 ~ +12 semitones)
- **시간 늘이기 (Time Stretch)**: 재생 속도 조절 (0.5x ~ 2.0x)
- **오디오 필터**: Low Pass, High Pass, Band Pass, Robot, Echo, Reverb
- **실시간 분석**: 피치 및 길이 분석
- **인터랙티브 편집**: 그래프 기반 직관적 편집

### 🔧 다양한 알고리즘 지원

#### Pitch Shift 알고리즘
- **FastPitchShift**: 단순 리샘플링 (가장 빠름)
- **HighQuality**: Phase Vocoder 기반 (고품질, 현재 오차 존재)
- **External**: SoundTouch 라이브러리 (권장, 균형잡힌 성능)
- **PSOLA**: Pitch Synchronous Overlap-Add (시간 도메인, 정확하지만 느림)
- **RubberBand**: 산업 표준 (최고 정확도, GPL)

#### Time Stretch 알고리즘
- **Fast**: 프레임 반복/스킵 (가장 빠름)
- **WSOLA**: Waveform Similarity Overlap-Add (고품질)
- **SoundTouch**: 외부 라이브러리 (권장)
- **Phase Vocoder**: STFT 기반 (매우 높은 품질)
- **RubberBand**: 산업 표준 (최고 품질, GPL)

### 📊 벤치마크 시스템
- **4가지 벤치마크 카테고리**:
  - **부분 구간 시간 늘이기**: 500ms, 1s, 2s 구간 테스트 (30개 조합)
  - **피치 변조**: 5개 알고리즘 (Fast, Phase Vocoder, SoundTouch, PSOLA, RubberBand)
  - **시간 늘이기**: 5개 알고리즘
  - **통합 처리**: 5개 방법
- **총 45개 테스트** 자동 실행
- **실시간 처리 가능 여부** 측정
- **품질 메트릭**: SNR, RMS Error, Pitch/Duration 정확도, 경계 불연속성
- **탭 방식 통합 리포트**: 한글로 작성된 상세 분석

## 🚀 시작하기

### 필수 요구사항

- **macOS** (Apple Silicon 권장)
- **Python 3**
- **Emscripten** (WebAssembly 컴파일)
- **clang++** (C++ 컴파일러)

### 설치 및 실행

```bash
# 1. 저장소 클론
git clone <repository-url>
cd school

# 2. 빌드 및 서버 실행 (자동으로 벤치마킹 수행)
./runserver.sh

# 3. 브라우저에서 접속
# - 메인 앱: http://localhost:8088/web/
# - 벤치마크: http://localhost:8088/web/benchmark/comprehensive_benchmark_report.html
```

### 개별 빌드

```bash
# WebAssembly 빌드만
./build.sh

# 벤치마크만 빌드 및 실행
cd tests
./build_all_benchmarks.sh
./run_all_benchmarks.sh
```

## 📁 프로젝트 구조

```
school/
├── src/                           # C++ 소스 코드
│   ├── audio/                     # 오디오 버퍼 관리
│   ├── analysis/                  # 피치/길이 분석
│   ├── preprocessor/              # 전처리기 (새로운 아키텍처)
│   │   ├── OutlierCorrector.cpp   # Gradient 기반 outlier 감지/보정
│   │   └── SplineInterpolator.cpp # Cubic Spline 보간
│   ├── processor/                 # 프로세서 (새로운 아키텍처)
│   │   ├── pitch/                 # Pitch Processors
│   │   │   ├── PSOLAPitchProcessor.cpp
│   │   │   ├── PhaseVocoderPitchProcessor.cpp
│   │   │   ├── SoundTouchPitchProcessor.cpp
│   │   │   └── RubberBandPitchProcessor.cpp
│   │   └── duration/              # Duration Processors
│   │       ├── WSOLADurationProcessor.cpp
│   │       ├── SoundTouchDurationProcessor.cpp
│   │       └── RubberBandDurationProcessor.cpp
│   ├── pipeline/                  # 파이프라인 (새로운 아키텍처)
│   │   ├── PitchFirstPipeline.cpp # Pitch → Duration 순서
│   │   └── HybridPipeline.cpp     # Preview/Final 모드
│   ├── algorithm/                 # Low-level 알고리즘
│   │   ├── pitch/                 # Pitch 알고리즘들
│   │   └── duration/              # Duration 알고리즘들
│   ├── effects/                   # 레거시 이펙트 (Algorithm에서 사용)
│   │   ├── PhaseVocoder.cpp       # Phase Vocoder 구현
│   │   ├── PhaseVocoderPitchShifter.cpp
│   │   └── VoiceFilter.cpp        # 음성 필터
│   ├── deprecated/                # Deprecated 코드 (사용 안 함)
│   │   └── effects/               # 구 Strategy 패턴 코드 (20개 파일)
│   ├── benchmark/                 # 벤치마크 시스템
│   │   ├── PartialSegmentBenchmark.cpp
│   │   ├── PitchShiftBenchmark.cpp
│   │   ├── TimeStretchBenchmark.cpp
│   │   └── CombinedBenchmark.cpp
│   ├── utils/                     # 유틸리티
│   └── external/                  # 외부 라이브러리
│       ├── soundtouch/            # SoundTouch 라이브러리
│       ├── kissfft/               # FFT 라이브러리
│       └── rubberband/            # RubberBand 라이브러리
├── web/                          # 웹 인터페이스
│   ├── index.html               # 메인 페이지
│   ├── main.js                  # WASM 바인딩
│   ├── js/                      # JavaScript 모듈
│   ├── css/                     # 스타일
│   └── benchmark/               # 벤치마크 결과 (심볼릭 링크)
├── tests/                        # 테스트 및 벤치마크
│   ├── benchmark_partialsegment  # 부분 구간 시간 늘이기 벤치마크
│   ├── benchmark_pitchshift      # 피치 변조 벤치마크
│   ├── benchmark_timestretch     # 시간 늘이기 벤치마크
│   ├── benchmark_combined        # 통합 처리 벤치마크
│   ├── build_all_benchmarks.sh   # 전체 빌드 스크립트
│   ├── run_all_benchmarks.sh     # 벤치마크 실행
│   └── create_tabbed_report.sh   # 통합 리포트 생성
├── benchmark_result/             # 벤치마크 결과
│   ├── comprehensive_benchmark_report.html  # 통합 리포트
│   ├── benchmark_*_report.json   # JSON 데이터
│   └── output_*.wav              # 벤치마크 오디오 출력
├── build.sh                      # WebAssembly 빌드 스크립트
├── runserver.sh                  # 서버 실행 스크립트
└── README.md                     # 이 파일
```

## 🎯 사용 방법

### 1. 음성 녹음 또는 업로드
- "녹음 시작" 버튼으로 마이크 녹음
- "파일 업로드" 버튼으로 WAV 파일 업로드

### 2. 음성 분석 및 편집
- **HighQuality**: 자체 구현 알고리즘
- **External**: SoundTouch 하이브리드
- **A/B 비교**: 두 알고리즘 동시 비교

### 3. 음성 효과 적용
- 피치 조절: 슬라이더로 반음 단위 조절
- 시간 늘이기: 0.5배 ~ 2.0배 속도 조절
- 필터: 다양한 오디오 효과

### 4. 재생 및 다운로드
- 변조된 음성 미리 듣기
- WAV 파일로 다운로드

## 🏗️ 아키텍처

### 새로운 Pipeline 아키텍처 (2025-11-12)

기존 Strategy 패턴을 **완전히 제거**하고 **Pipeline 아키텍처**로 전환했습니다:

```
User Input (Edit Points)
         ↓
  [Preprocessors]
    - OutlierCorrector (Gradient 기반)
    - SplineInterpolator (Cubic Spline)
         ↓
  Interpolated Frames
         ↓
    [Pipeline]
         ↓
  [Pitch Processor] → [Duration Processor]
         ↓
   AudioBuffer (Output)
```

**주요 개선사항**:
- ✅ 모듈화된 구조
- ✅ 16가지 알고리즘 조합 (4 Pitch × 4 Duration)
- ✅ 확장 가능한 설계
- ✅ 315줄 코드 제거

**상세 문서**:
- [아키텍처 가이드](docs/NEW_ARCHITECTURE.md)
- [구현 완료 보고서](docs/IMPLEMENTATION_SUMMARY.md)
- [Quick Start](docs/QUICK_START.md)

## 📊 벤치마크 결과

### 속도 비교 (4.69초 오디오 기준)

| 알고리즘 | Pitch Shift | Time Stretch | Combined |
|---------|-------------|--------------|----------|
| Fast    | 0.35 ms     | 0.55 ms      | -        |
| SoundTouch | 25.9 ms | 30.87 ms     | 40.94 ms |
| WSOLA   | -           | 108.78 ms    | -        |
| PSOLA   | 1448.6 ms   | -            | -        |
| Phase Vocoder | 36.89 ms | 672.49 ms | 711.84 ms |
| RubberBand | 171.7 ms | 184.50 ms    | 211.92 ms |

### 부분 구간 처리 (실제 사용 시나리오)

전체 오디오가 아닌 **특정 구간만 처리**하는 실제 사용 사례 테스트:

| 구간 길이 | 비율 | Fast | WSOLA | SoundTouch | Phase Vocoder | RubberBand |
|-----------|------|------|-------|------------|---------------|------------|
| 500ms     | 0.75x (빠르게) | 0.3ms | 19ms | 9ms | 149ms | 42ms |
| 500ms     | 1.5x (느리게) | 0.3ms | 20ms | 10ms | 150ms | 43ms |
| 1s        | 0.75x | 0.3ms | 38ms | 19ms | 302ms | 83ms |
| 1s        | 1.5x | 0.3ms | 40ms | 20ms | 304ms | 85ms |
| 2s        | 0.75x | 0.3ms | 76ms | 38ms | 603ms | 166ms |
| 2s        | 1.5x | 0.3ms | 77ms | 39ms | 605ms | 168ms |

**주요 발견사항:**
- SoundTouch: 짧은 구간에서도 안정적 성능 (10-40ms)
- Fast: 매우 빠르지만 경계 불연속성 높음 (0.04-0.06)
- WSOLA/RubberBand: 중간 성능, 경계 품질 우수
- Phase Vocoder: 높은 품질이지만 느림 (150-600ms)

### 피치 변조 정확도 비교 (+3 semitones 테스트)

| 알고리즘 | 처리 시간 | 피치 오차 | Duration Ratio | 특징 |
|---------|----------|----------|----------------|------|
| Fast    | 0.35 ms  | -0.064 st | 0.841 | 가장 빠름, 시간 변화 |
| SoundTouch | 25.9 ms | -0.084 st | 1.0 | 균형잡힌 성능 |
| Phase Vocoder | 36.9 ms | +14.77 st | 1.0 | 현재 오차 존재 |
| PSOLA | 1448.6 ms | -0.020 st | 0.839 | 정확하지만 매우 느림 |
| **RubberBand** | **171.7 ms** | **-0.001 st** | **1.0** | **최고 정확도** |

### 권장 사항

- **실시간 처리**: SoundTouch (빠르고 정확, 25ms)
- **최고 정확도**: RubberBand (0.001 semitones 오차, GPL 주의)
- **모바일/임베디드**: Fast (0.35ms, 단 시간 변화 발생)
- **시간 도메인 처리**: PSOLA (정확하지만 느림, 1.4초)
- **오디오 마스터링**: RubberBand (GPL 주의)

## 🔬 기술 스택

- **C++17**: 코어 오디오 처리
- **WebAssembly (Emscripten)**: 웹 포팅
- **JavaScript/HTML5**: 웹 인터페이스
- **D3.js**: 데이터 시각화
- **FFT**: KissFFT 라이브러리
- **외부 라이브러리**:
  - SoundTouch (LGPL 2.1)
  - RubberBand (GPL 2.0)

## 📈 알고리즘 설명

### Phase Vocoder
STFT(Short-Time Fourier Transform)를 사용하여 주파수 도메인에서 오디오를 처리합니다. 매우 높은 품질을 제공하지만 처리 속도가 느립니다.

### WSOLA (Waveform Similarity Overlap-Add)
시간 도메인에서 유사한 파형을 찾아 오버랩하는 방식으로 시간을 늘입니다. 중간 수준의 품질과 속도를 제공합니다.

### SoundTouch
검증된 오픈소스 라이브러리로, WSOLA 기반의 최적화된 알고리즘을 제공합니다. 실시간 처리에 가장 적합합니다.

### PSOLA (Pitch Synchronous Overlap-Add)
시간 도메인에서 동작하는 피치 변조 알고리즘입니다. 자기상관(Autocorrelation)을 사용하여 피치 주기를 검출하고, 각 주기마다 윈도우를 적용하여 오버랩-애드 방식으로 피치를 변조합니다. FFT가 불필요하여 메모리 효율적이지만, 처리 속도가 느립니다.

### RubberBand
전문 오디오 편집 소프트웨어에서 사용되는 산업 표준 라이브러리입니다. 피치 변조와 시간 늘이기를 독립적으로 제어할 수 있으며, 최고 수준의 정확도를 제공합니다. GPL 라이선스입니다.

## 🐛 알려진 이슈

1. **Phase Vocoder Pitch Shift 오차**: 현재 Phase Vocoder를 사용한 피치 변조에서 큰 오차가 발생합니다 (~14 semitones). SoundTouch나 RubberBand 사용을 권장합니다.

2. **PSOLA 처리 속도**: PSOLA는 정확한 피치 변조를 제공하지만 처리 속도가 매우 느립니다 (4.7초 오디오에 1.4초). 실시간 처리에는 부적합합니다.

3. **RubberBand 버퍼 경고**: RubberBand 사용 시 버퍼 크기 경고가 표시될 수 있으나 정상 작동합니다.

## 📝 라이선스

이 프로젝트는 다음 오픈소스 라이브러리를 사용합니다:

- **SoundTouch**: LGPL 2.1
- **RubberBand**: GPL 2.0
- **KissFFT**: BSD 3-Clause

RubberBand는 GPL 라이선스이므로 상업적 사용 시 주의가 필요합니다.

## 🤝 기여

버그 리포트, 기능 제안, 풀 리퀘스트를 환영합니다!

## 📞 문의

프로젝트 관련 문의사항은 이슈 트래커를 이용해주세요.

---

**마지막 업데이트**: 2025-01-12
**버전**: 1.0.0
