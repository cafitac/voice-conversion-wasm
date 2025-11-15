# Quick Start Guide - Variable Pitch/Duration 시스템

## 5분 안에 시작하기

### 1. 빌드

```bash
# Emscripten 설치 (최초 1회)
./setup_emscripten.sh

# 빌드
./build.sh

# 웹 서버 실행
./runserver.sh
```

브라우저에서 `http://localhost:8000` 접속

### 2. 기본 사용법

#### 2.1 오디오 로드

1. 웹 페이지에서 "파일 선택" 버튼 클릭
2. WAV 파일 선택
3. 자동으로 분석 시작

#### 2.2 Pitch 편집

1. **그래프에서 클릭**하여 편집 포인트 추가
2. 편집 포인트를 **드래그**하여 위치/값 조정
3. 편집 포인트를 **우클릭**하여 삭제

#### 2.3 알고리즘 선택

**Pitch 알고리즘**:
- `PSOLA` - 빠른 미리듣기 (1-2초)
- `Phase Vocoder` - 고품질 (5-10초) ⭐ 권장
- `SoundTouch` - 안정적
- `RubberBand` - 최고 품질 (느림)

**Duration 알고리즘**:
- `없음` - Duration 처리 안 함
- `WSOLA` - 빠른 처리
- `SoundTouch` - 안정적 ⭐ 권장
- `RubberBand` - 최고 품질

#### 2.4 샘플 생성 및 재생

1. "샘플 생성" 버튼 클릭
2. 처리 완료 후 "샘플 재생" 버튼으로 미리듣기
3. 만족하면 "다운로드" 버튼으로 저장

## 주요 기능

### Gradient 기반 Outlier Correction

**자동으로 적용됩니다!**

- 급격한 변화(outlier) 자동 감지
- 인접 프레임을 참고하여 자연스럽게 보정
- 그래프에서 빨간색 점으로 표시됨

**조정 방법**: `gradientThreshold` 값 변경 (기본 3.0)
- 높이면: 더 적은 outlier 감지 (더 공격적인 변화 허용)
- 낮추면: 더 많은 outlier 감지 (더 부드러운 곡선)

### Cubic Spline Interpolation

**자동으로 적용됩니다!**

- 편집 포인트 사이를 부드럽게 보간
- C2 연속성 보장 (2차 미분까지 연속)
- 자연스러운 곡선 생성

**조정 방법**: `frameInterval` 값 변경 (기본 0.02 = 20ms)
- 줄이면: 더 정밀한 보간 (더 많은 프레임)
- 늘리면: 더 빠른 처리 (더 적은 프레임)

## 일반적인 워크플로우

### 음성 편집 (권장)

1. WAV 파일 로드
2. 그래프에서 Pitch 편집
3. 알고리즘 선택:
   - Pitch: `Phase Vocoder` (고품질)
   - Duration: `없음` (또는 `SoundTouch`)
4. 샘플 생성 및 확인
5. 필요시 수정 후 재생성
6. 다운로드

### 음악 편집

1. WAV 파일 로드
2. 그래프에서 Pitch 편집
3. 알고리즘 선택:
   - Pitch: `RubberBand` (최고 품질)
   - Duration: `RubberBand`
4. **주의**: 처리 시간이 오래 걸림 (3분 → 30초)
5. 샘플 생성 및 확인
6. 다운로드

### 실시간 미리듣기

1. WAV 파일 로드
2. 알고리즘 선택:
   - Pitch: `PSOLA` (가장 빠름)
   - Duration: `WSOLA`
3. 편집하면서 빠르게 샘플 생성
4. 최종 확정 시 고품질 알고리즘으로 변경하여 재생성

## 팁과 트릭

### 더 나은 결과를 위한 팁

1. **편집 포인트를 적절히 배치**
   - 너무 급격한 변화는 artifacts 발생 가능
   - 부드러운 곡선을 만드세요

2. **Outlier 활용**
   - 그래프에서 빨간색 점을 확인하세요
   - 의도하지 않은 급격한 변화를 알려줍니다

3. **알고리즘 선택**
   - 실험할 때: PSOLA (빠름)
   - 최종 출력: Phase Vocoder (품질)
   - 음악: RubberBand (최고 품질)

4. **Duration은 필요할 때만**
   - Duration 처리는 추가 시간이 걸립니다
   - Pitch만 변경하려면 `없음` 선택

### 성능 최적화

**처리 시간이 너무 길 때**:
- PSOLA로 변경
- Duration을 `없음`으로 설정
- `frameInterval`을 늘림 (0.02 → 0.05)

**품질이 만족스럽지 않을 때**:
- Phase Vocoder나 RubberBand로 변경
- `frameInterval`을 줄임 (0.02 → 0.01)
- 편집 포인트를 더 세밀하게 배치

## 문제 해결

### "빌드 실패"

```bash
# Emscripten 재설정
rm -rf emsdk/
./setup_emscripten.sh
./build.sh
```

### "웹 페이지가 로드되지 않음"

1. 빌드가 완료되었는지 확인
2. `web/main.js`와 `web/main.wasm` 파일이 있는지 확인
3. 브라우저 콘솔에서 에러 확인

### "Outlier가 너무 많음"

`gradientThreshold`를 높이세요:
- 기본값: 3.0
- 추천값: 4.0 ~ 5.0

JavaScript 코드에서:
```javascript
const interpolated = Module.preprocessAndInterpolate(
    duration, sampleRate, editPoints,
    5.0,   // gradientThreshold 증가
    0.02
);
```

### "보간 곡선이 부자연스러움"

1. 편집 포인트를 더 부드럽게 배치
2. `frameInterval`을 줄여서 더 정밀하게 (0.02 → 0.01)

### "처리가 중단됨"

1. 브라우저 콘솔 확인
2. 오디오 파일이 올바른 WAV 포맷인지 확인
3. 메모리 부족일 수 있음 (긴 오디오 파일의 경우)

## 다음 단계

- [상세 아키텍처 문서](NEW_ARCHITECTURE.md) 읽기
- [Deprecated 마이그레이션 가이드](../src/deprecated/README.md) 확인
- 예제 코드 실험

## 지원

- GitHub Issues: 버그 리포트
- 문서: `/docs` 폴더
- 예제: `/web/js/ui-controller.js`
