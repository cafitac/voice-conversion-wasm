# 🎵 Voice Conversion WASM

C++ WebAssembly 기반 실시간 음성 변조 웹 애플리케이션

[![Deploy](https://github.com/cafitac/voice-conversion-wasm/actions/workflows/deploy.yml/badge.svg)](https://github.com/cafitac/voice-conversion-wasm/actions/workflows/deploy.yml)
[![Live Demo](https://img.shields.io/badge/demo-live-success)](https://voice-conversion-wasm.vercel.app/app/index.html)

---

## 📖 프로젝트 소개

실시간으로 음성을 변조하는 웹 애플리케이션입니다. C++ WebAssembly를 사용하여 고성능 오디오 처리를 구현했으며, JavaScript 엔진과의 성능 비교 기능을 제공합니다.

### ✨ 주요 기능

- 🎤 **실시간 녹음**: 마이크로 음성 녹음
- 📂 **파일 업로드**: WAV 파일 업로드 및 처리
- 🎚️ **피치 변환**: -12 ~ +12 반음 조절
- ⏱️ **속도 조절**: 0.5배 ~ 2.0배 시간 늘리기/줄이기
- 🎛️ **음성 필터**: 12가지 음성 효과 (로봇, 메아리, 무전기 등)
- ⏮️ **역재생**: 오디오 역방향 재생
- 📊 **성능 비교**: C++ vs JavaScript 실시간 벤치마크
- 💾 **다운로드**: 변환된 오디오를 WAV 파일로 저장

### 🚀 라이브 데모

👉 **[https://voice-conversion-wasm.vercel.app/app/index.html](https://voice-conversion-wasm.vercel.app/app/index.html)**

---

## 🏗️ 기술 스택

### Core
- **C++ (17)**: 고성능 DSP 알고리즘 구현
- **WebAssembly**: C++ 코드를 웹에서 실행
- **Emscripten**: C++ → WASM 컴파일러

### Frontend
- **Vanilla JavaScript**: 프레임워크 없는 순수 JavaScript
- **Web Audio API**: 오디오 입출력 처리
- **D3.js**: 실시간 피치 시각화

### Build & Deploy
- **GitHub Actions**: 자동 빌드 및 배포
- **Vercel**: 정적 사이트 호스팅

---

## 📂 프로젝트 구조

```
school/
├── src/                          # C++ 소스 코드
│   ├── audio/                    # 오디오 기본 기능
│   │   ├── AudioBuffer.cpp       # 오디오 데이터 컨테이너
│   │   ├── AudioPreprocessor.cpp # 전처리 (프레임 분할, 윈도우)
│   │   └── BufferPool.h          # 메모리 풀링
│   ├── dsp/                      # 디지털 신호 처리
│   │   ├── SimpleTimeStretcher.cpp  # WSOLA 시간 늘이기/줄이기
│   │   └── SimplePitchShifter.cpp   # 피치 변환 (Time Stretch + Resampling)
│   ├── effects/                  # 음성 효과
│   │   ├── VoiceFilter.cpp       # 12가지 음성 필터
│   │   └── AudioReverser.cpp     # 역재생
│   ├── analysis/                 # 분석 알고리즘
│   │   └── PitchAnalyzer.cpp     # YIN 알고리즘 피치 분석
│   ├── performance/              # 성능 측정
│   │   └── PerformanceChecker.cpp
│   └── main.cpp                  # Emscripten 바인딩
│
├── web/                          # 웹 프론트엔드
│   ├── app/                      # 메인 애플리케이션
│   │   ├── index.html            # 메인 페이지
│   │   ├── css/style.css         # 스타일
│   │   └── js/
│   │       ├── UnifiedController.js  # C++/JS 엔진 통합 컨트롤러
│   │       └── PerformanceReport.js  # 성능 보고서
│   └── js/js/                    # JavaScript 엔진 (C++ 동일 알고리즘)
│       ├── audio/                # AudioBuffer, AudioRecorder, AudioPlayer
│       ├── dsp/                  # SimplePitchShifter, SimpleTimeStretcher
│       ├── effects/              # VoiceFilter, AudioReverser
│       ├── analysis/             # PitchAnalyzer
│       └── utils/                # WavEncoder
│
├── dist/                         # 빌드 출력 (Vercel 배포용)
│   ├── main.js                   # WASM 로더
│   ├── main.wasm                 # 컴파일된 WebAssembly
│   └── app/                      # 웹 앱 파일
│
├── tests/                        # 테스트 코드
├── docs/                         # 문서
│
├── build.sh                      # WASM 빌드 스크립트
├── build-dist.sh                 # 배포용 빌드 스크립트
├── runserver.sh                  # 로컬 서버 실행
└── watch.sh                      # 파일 변경 감지 및 자동 빌드
```

---

## 🛠️ 로컬 개발 환경 설정

### 1. 사전 요구사항

- **macOS** / **Linux** (Windows는 WSL 권장)
- **Python 3.x** (로컬 서버용)
- **Git**

### 2. 설치

```bash
# 저장소 클론
git clone https://github.com/cafitac/voice-conversion-wasm.git
cd voice-conversion-wasm

# Emscripten 설치 (자동)
./build.sh  # 첫 실행 시 emsdk 자동 설치 및 활성화
```

### 3. 빌드

```bash
# WebAssembly 빌드
./build.sh

# 또는 개발 모드 (파일 변경 감지 + 자동 빌드 + 서버 실행)
./watch.sh
```

### 4. 실행

```bash
# 로컬 서버 시작
./runserver.sh

# 브라우저에서 열기
# http://localhost:8000/app/index.html
```

---

## 🎯 주요 알고리즘

### 1. WSOLA (Waveform Similarity Overlap-Add)
**파일**: `src/dsp/SimpleTimeStretcher.cpp`

피치를 유지하면서 오디오 속도를 변경하는 알고리즘

**핵심 원리**:
1. 오디오를 작은 세그먼트로 분할
2. 세그먼트를 간격 조정하여 배치
3. 겹치는 부분에서 가장 유사한 위치 찾기 (상관관계 계산)
4. 크로스페이드로 부드럽게 연결

**최적화**:
- Loop Unrolling (4-way) → 컴파일러 자동 벡터화 유도
- Early Exit (상관관계 0.95 이상이면 즉시 종료)
- Coarse-to-Fine 검색 (2샘플씩 건너뛰며 빠른 탐색 → 정밀 탐색)

### 2. Pitch Shifting (Time Stretch + Resampling)
**파일**: `src/dsp/SimplePitchShifter.cpp`

길이를 유지하면서 피치를 변경하는 알고리즘

**핵심 원리**:
1. Time Stretch: 속도 변경 (피치도 함께 변함)
2. Resampling: 원래 길이로 복원 (피치만 변경됨)

**예시**:
- 피치 +5 반음: 느리게 만들고(1/1.33) → 빠르게 재생(1.33)
- 피치 -5 반음: 빠르게 만들고(1.33) → 느리게 재생(1/1.33)

### 3. YIN 알고리즘 (Pitch Detection)
**파일**: `src/analysis/PitchAnalyzer.cpp`

오디오에서 주파수를 탐지하는 알고리즘

**핵심 원리**:
1. 자기 상관 함수 (Autocorrelation) 계산
2. 차분 함수 (Difference Function) 변환
3. 누적 평균 정규화 (CMNDF)
4. 임계값 기반 피치 후보 선택
5. 포물선 보간 (Parabolic Interpolation)으로 정밀도 향상

---

## ⚡ 성능 최적화

자세한 내용은 **[OPTIMIZATION.md](./OPTIMIZATION.md)** 참고

### 주요 최적화 기법

1. **Zero-Copy 메모리 전달**
   - `typed_memory_view`로 JavaScript ↔ C++ 복사 제거
   - **효과**: 2-3배 빠름

2. **Loop Unrolling + 자동 벡터화**
   - 4-way Loop Unrolling으로 루프 오버헤드 감소
   - 컴파일러가 SIMD 명령어로 자동 변환
   - **효과**: 1.3-2배 빠름

3. **알고리즘 최적화**
   - Early Exit: 불필요한 계산 50% 감소
   - Coarse-to-Fine: 검색 횟수 50% 감소
   - **효과**: 1.5-2배 빠름

4. **메모리 풀링**
   - 버퍼 재사용으로 할당/해제 오버헤드 감소
   - **효과**: 1.1-1.3배 빠름

### 최종 결과
- **C++이 JavaScript보다 2-6배 빠른 성능**
- Duration 조절: **140배 빠름** (27ms vs 3824ms)
- Pitch 조절: **4-5배 빠름** (700-800ms vs 3500ms)

---

## 📊 사용 가이드

### 기본 사용법

1. **음성 입력**
   - 🔴 녹음 버튼: 마이크로 실시간 녹음
   - 📂 업로드 버튼: WAV 파일 업로드

2. **효과 적용**
   - 피치 슬라이더: -12 ~ +12 반음 조절
   - 속도 슬라이더: 0.5배 ~ 2.0배
   - 필터 선택: 12가지 효과 중 선택
   - 역재생 체크박스: 오디오 거꾸로 재생

3. **변환 및 재생**
   - "변환" 버튼 클릭
   - ▶ 버튼으로 변환된 오디오 재생
   - ↓ 버튼으로 WAV 파일 다운로드

### 성능 측정

1. 📊 버튼 클릭 → 성능 보고서 패널 열기
2. **비교 탭**: C++ vs JavaScript 성능 비교표
3. **C++ 탭**: C++ 엔진 상세 성능 (함수별 시간)
4. **JavaScript 탭**: JavaScript 엔진 상세 성능
5. **보고서 목록 탭**: 과거 측정 기록 보기
6. **JSON/CSV 다운로드**: 데이터 내보내기

---

## 🧪 테스트

```bash
# 피치 분석 테스트
./tests/build_pitch_analyzer_test.sh
./tests/test_pitch_analyzer

# 재구성 테스트
./tests/build_reconstruction_test.sh
./tests/test_reconstruction

# 편집 파이프라인 테스트
./tests/build_edit_pipeline_test.sh
./tests/test_edit_pipeline
```

---

## 📚 문서

- **[COMPONENTS_GUIDE.md](./COMPONENTS_GUIDE.md)** - 전체 컴포넌트 상세 가이드
- **[OPTIMIZATION.md](./OPTIMIZATION.md)** - 성능 최적화 기법 및 원리
- **[PRESENTATION_GUIDE.md](./PRESENTATION_GUIDE.md)** - 프레젠테이션 가이드
- **[QUICK_REFERENCE.md](./QUICK_REFERENCE.md)** - 빠른 참조 가이드

---

## 🚀 배포

### GitHub Actions 자동 배포

1. **코드 푸시**
   ```bash
   git add .
   git commit -m "feat: 새 기능 추가"
   git push origin main
   ```

2. **자동 빌드**
   - GitHub Actions가 자동으로 WebAssembly 빌드
   - `dist/` 폴더에 결과물 생성 및 커밋

3. **Vercel 자동 배포**
   - `dist/` 폴더 변경 감지
   - https://voice-conversion-wasm.vercel.app 자동 업데이트

### 수동 배포

```bash
# 배포용 빌드
./build-dist.sh

# Vercel CLI로 배포 (옵션)
vercel --prod
```

---

## 🛠️ 빌드 스크립트

### build.sh
WebAssembly 빌드 (개발용)

```bash
./build.sh
```

**컴파일 옵션**:
- `-O3`: 최고 수준 최적화
- `-msimd128`: WASM SIMD 활성화
- `-ffast-math`: 부동소수점 최적화
- `--bind`: Emscripten 바인딩

### build-dist.sh
배포용 빌드 (GitHub Actions에서 사용)

```bash
./build-dist.sh
```

**추가 작업**:
- `dist/` 폴더 생성
- 웹 파일 복사 (HTML, CSS, JS)
- WASM 파일 복사

### watch.sh
파일 변경 감지 및 자동 빌드

```bash
./watch.sh
```

**기능**:
- `src/`, `web/` 폴더 감시
- 파일 변경 시 자동 빌드
- 로컬 서버 자동 시작

---

## 🤝 기여하기

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📝 라이선스

이 프로젝트는 MIT 라이선스를 따릅니다.

---

## 👤 작성자

**cafitac**

- GitHub: [@cafitac](https://github.com/cafitac)

---

## 🙏 감사의 말

- **Emscripten**: C++ to WebAssembly 컴파일러
- **SoundTouch**: 오디오 처리 라이브러리 (참고용)
- **YIN 알고리즘**: Alain de Cheveigné and Hideki Kawahara
- **WSOLA 알고리즘**: Werner Verhelst and Marc Roelands

---

## 📌 참고 자료

### WebAssembly & Emscripten
- [Emscripten 공식 문서](https://emscripten.org/docs/)
- [WebAssembly 공식 사이트](https://webassembly.org/)
- [WASM SIMD](https://v8.dev/features/simd)

### 오디오 처리 알고리즘
- [WSOLA 논문](https://ieeexplore.ieee.org/document/655632)
- [YIN 알고리즘 논문](http://audition.ens.fr/adc/pdf/2002_JASA_YIN.pdf)
- [Web Audio API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)

### 성능 최적화
- [Loop Unrolling](https://en.wikipedia.org/wiki/Loop_unrolling)
- [SIMD Programming](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [C++ Move Semantics](https://en.cppreference.com/w/cpp/language/move_constructor)

---

**⭐ 이 프로젝트가 도움이 되었다면 Star를 눌러주세요!**
