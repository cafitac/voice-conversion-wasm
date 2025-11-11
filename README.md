# 음성 변조 웹 앱 (Voice Modulation Web App)

C++/WebAssembly 기반의 고품질 음성 변조 애플리케이션입니다.
Pitch Shift와 Time Stretch 알고리즘을 Strategy Pattern으로 구현하여 다양한 품질/속도 옵션을 제공합니다.

## ✨ 주요 기능

- 🎵 **Pitch Shift**: 음높이 조절 (±12 semitones)
- ⏱️ **Time Stretch**: 재생 속도 조절 (0.5x ~ 2.0x)
- 🎛️ **음성 필터**: Low/High/Band Pass, Robot, Echo, Reverb
- 📊 **실시간 분석**: Pitch, Duration, Power 분석 및 시각화
- 🔊 **고품질 처리**: SoundTouch 라이브러리 + 자체 알고리즘

## 🏗️ Strategy Pattern 아키텍처

각 효과는 3가지 전략(Strategy)으로 구현되어 있습니다:

| 전략 | 구현 방식 | 특징 |
|------|-----------|------|
| **Fast** | 간단한 알고리즘 | 매우 빠름, 낮은 품질 |
| **HighQuality** | 자체 알고리즘 (WSOLA, Phase Vocoder) | 외부 의존성 없음, 고품질 |
| **External** | SoundTouch 라이브러리 | 검증된 프로덕션 품질 |

### 📊 성능 비교 (TimeStretch 기준)

| 전략 | 처리 속도 | Pitch 보존 | 품질 |
|------|-----------|------------|------|
| Fast | 0.5ms | ❌ -32% | 낮음 |
| HighQuality | 106ms | ✅ +2.28% | 우수 |
| External | 30ms | ✅ +0.13% | 최고 |

자세한 분석: [`TIME_STRETCH_QUALITY_ANALYSIS.md`](TIME_STRETCH_QUALITY_ANALYSIS.md)

## 필요 도구

1. **Emscripten SDK** 설치
   ```bash
   # Emscripten SDK 다운로드
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk

   # 최신 버전 설치
   ./emsdk install latest
   ./emsdk activate latest

   # 환경 변수 설정
   source ./emsdk_env.sh
   ```

## 빌드 및 실행 방법

1. 스크립트 실행 권한 부여:
   ```bash
   chmod +x runserver.sh
   ```

2. 서버 실행 (자동으로 빌드 후 서버 시작):
   ```bash
   ./runserver.sh
   ```

3. 브라우저에서 접속:
   ```
   http://localhost:8088/web/
   ```

`runserver.sh` 스크립트는 자동으로 빌드를 실행한 후 웹 서버를 시작합니다.

### 빌드만 실행하기

빌드만 따로 실행하려면:
```bash
./build.sh
```

## 📁 프로젝트 구조

```
school/
├── src/
│   ├── audio/              # 오디오 처리 기본 클래스
│   ├── analysis/           # Pitch, Duration, Power 분석
│   ├── effects/            # 효과 (Strategy Pattern)
│   │   ├── *PitchShiftStrategy.{h,cpp}    # Pitch shift 전략들
│   │   ├── *TimeStretchStrategy.{h,cpp}   # Time stretch 전략들
│   │   └── VoiceFilter.{h,cpp}            # 음성 필터
│   ├── synthesis/          # 프레임 재구성
│   ├── utils/              # 유틸리티 (WAV, FFT)
│   └── visualization/      # 캔버스 렌더링
├── web/
│   ├── index.html          # 웹 인터페이스
│   ├── css/style.css       # 모던 다크 테마
│   └── js/ui-controller.js # UI 컨트롤러
├── tests/
│   ├── benchmark_pitchshift.cpp   # Pitch shift 벤치마크
│   ├── benchmark_timestretch.cpp  # Time stretch 벤치마크
│   └── benchmark_combined.cpp     # Combined 벤치마크
├── QUALITY_ANALYSIS.md            # Pitch shift 품질 분석
├── TIME_STRETCH_QUALITY_ANALYSIS.md  # Time stretch 품질 분석
├── COMBINED_QUALITY_ANALYSIS.md      # Combined 품질 분석
└── DEVELOPMENT_RULES.md           # 개발 규칙
```
## 🧪 벤치마크

각 알고리즘의 성능을 테스트할 수 있습니다:

```bash
cd tests

# Time Stretch 벤치마크
./build_benchmark_timestretch.sh
./benchmark_timestretch benchmark_external.wav 1.5

# Pitch Shift 벤치마크
./build_benchmark_pitchshift.sh
./benchmark_pitchshift original.wav 3

# Combined (Pitch + Duration) 벤치마크
./build_benchmark_combined.sh
./benchmark_combined benchmark_fast.wav 3.0 1.5
```

벤치마크 결과는 자동으로 WAV 파일로 저장되며, 품질 분석 문서에서 확인할 수 있습니다.

## 📋 개발 규칙

이 프로젝트는 엄격한 개발 규칙을 따릅니다:

### 🔴 핵심 원칙

1. **Fast 전략**: 간단한 알고리즘만 사용, 외부 라이브러리 금지
2. **HighQuality 전략**: 자체 알고리즘 구현, 외부 라이브러리 금지
3. **External 전략**: SoundTouch만 사용, 자체 알고리즘 구현 금지

### 🔄 코드 수정 프로세스

```
수정 전 벤치마크 → 코드 수정 → 빌드 → 벤치마크 재실행
  ↓
✅ 개선됨? → 보고서 업데이트 → 커밋
❌ 악화됨? → 즉시 롤백 → 다른 방법 시도
```

**벤치마크 없이 커밋 절대 금지!**

자세한 내용: [`DEVELOPMENT_RULES.md`](DEVELOPMENT_RULES.md)

## 📚 문서

- [`DEVELOPMENT_RULES.md`](DEVELOPMENT_RULES.md) - 개발 규칙 및 가이드라인
- [`QUALITY_ANALYSIS.md`](QUALITY_ANALYSIS.md) - Pitch Shift 품질 분석
- [`TIME_STRETCH_QUALITY_ANALYSIS.md`](TIME_STRETCH_QUALITY_ANALYSIS.md) - Time Stretch 품질 분석
- [`COMBINED_QUALITY_ANALYSIS.md`](COMBINED_QUALITY_ANALYSIS.md) - 결합 처리 품질 분석
- [`RECONSTRUCTION_GUIDE.md`](RECONSTRUCTION_GUIDE.md) - Phase reconstruction 가이드

## 🎯 권장 설정

### 프로덕션 환경
```javascript
// 최고 품질 (권장)
Module.setPitchShiftQuality('external');
Module.setTimeStretchQuality('external');
```

### 외부 라이브러리 사용 불가 시
```javascript
// 자체 구현 (외부 의존성 없음)
Module.setPitchShiftQuality('high');
Module.setTimeStretchQuality('high');
```

## 🛠️ 기술 스택

- **언어**: C++17
- **빌드**: Emscripten (WebAssembly)
- **외부 라이브러리**: SoundTouch (LGPL)
- **DSP**: 자체 구현 Phase Vocoder, WSOLA
- **FFT**: KissFFT
- **디자인**: 모던 다크 테마 (Inter 폰트)

## 📄 라이선스

이 프로젝트는 교육 목적으로 제작되었습니다.
- SoundTouch 라이브러리: LGPL License

## 코드 수정 후

코드를 수정한 후에는 서버를 재시작하세요 (Ctrl+C로 종료 후 다시 실행):
```bash
./runserver.sh
```

## 테스트

### Pitch 분석 테스트

```bash
./tests/build_test.sh
./tests/test_pitch_analyzer original.wav
```

결과 파일: `tests/pitch_analysis.csv`

### FrameData 재구성 테스트

```bash
./tests/build_reconstruction_test.sh
./tests/test_reconstruction
```

생성된 파일:
- `tests/reconstructed.wav` - 재구성 품질 확인
- `tests/pitch_shifted.wav` - Pitch +2 semitones
- `tests/time_stretched.wav` - Duration 1.2배
- `tests/combined_modified.wav` - 복합 수정

더 자세한 정보는 [RECONSTRUCTION_GUIDE.md](./RECONSTRUCTION_GUIDE.md)를 참고하세요.
