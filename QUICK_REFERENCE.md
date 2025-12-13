# 빠른 참조 시트 (Quick Reference)

> 발표 중 빠르게 찾아볼 수 있는 1페이지 치트시트

---

## 🎯 핵심 개념 (30초 설명)

### Time Stretching (WSOLA)
**목적**: 음높이 유지하면서 길이만 변경
**방법**: 오디오를 40ms 세그먼트로 나누고 → 최적 위치 찾아 연결 → 크로스페이드

### Pitch Shifting
**목적**: 길이 유지하면서 음높이만 변경
**방법**: Time Stretch (1/ratio) → Resample (ratio)

---

## 📁 핵심 파일 위치

```
src/dsp/SimpleTimeStretcher.cpp:31-132       process() - WSOLA 메인
src/dsp/SimpleTimeStretcher.cpp:187-254      findBestOverlapPosition() - 최적 위치 탐색
src/dsp/SimplePitchShifter.cpp:24-59         process() - Pitch shift 메인
src/dsp/SimplePitchShifter.cpp:73-152        resample() - Linear interpolation
src/audio/BufferPool.h:14-74                 메모리 풀링
src/performance/PerformanceChecker.h         성능 측정
```

---

## ⚙️ 주요 파라미터

```cpp
// SimpleTimeStretcher.cpp:24-28
sequenceMs = 40;      // 세그먼트 크기 (40ms)
seekWindowMs = 15;    // 탐색 윈도우 (15ms) - 품질↑시 25ms
overlapMs = 8;        // 오버랩 크기 (8ms) - 부드러움↑시 12ms
```

---

## 🔢 핵심 수식

### Pitch → Frequency Ratio
```
ratio = 2^(semitones / 12)

예시:
  +12 semitones = 2.0 (옥타브 위)
  -12 semitones = 0.5 (옥타브 아래)
  +7 semitones = 1.498 (완전5도)
```

### Normalized Cross-Correlation
```
            Σ(buf1[i] × buf2[i])
corr = ───────────────────────────
        √(Σ buf1² × Σ buf2²)

범위: -1.0 ~ 1.0
1.0 = 완전 일치
```

### Linear Interpolation
```
pos = 1.7
index = 1, frac = 0.7

output = samples[1] × 0.3 + samples[2] × 0.7
```

---

## 🚀 성능 수치

| 항목 | 값 | 비고 |
|------|-----|------|
| **처리 속도** | 10-50ms | 2초 오디오 기준 |
| **메모리** | ~200KB | BufferPool 재사용 |
| **SIMD 가속** | 2-4배 | 4-way 언롤링 |
| **WASM Zero-Copy** | 2-3배 | typed_memory_view |

---

## 📊 알고리즘 비교

| 알고리즘 | 속도 | 품질 | 복잡도 | 선택 이유 |
|---------|------|------|--------|-----------|
| **WSOLA** (우리) | ⚡⚡⚡ | ⭐⭐⭐ | 낮음 | WebAssembly 최적 |
| **SoundTouch** | ⚡⚡ | ⭐⭐⭐⭐ | 중간 | 비교 벤치마크 |
| **Phase Vocoder** | ⚡ | ⭐⭐⭐⭐⭐ | 높음 | 미구현 (향후) |

---

## 🎵 품질 가이드

### 권장 사용 범위
```
안전: ±7 semitones (완전5도 이내)
허용: ±12 semitones (옥타브)
위험: ±12 초과 (심각한 아티팩트)

Time Stretch:
안전: 0.5 ~ 2.0
위험: < 0.5 or > 2.0
```

### 주요 아티팩트

| 문제 | 원인 | 해결 |
|------|------|------|
| **Phasiness** (위상 왜곡) | 최적 위치 못 찾음 | seekWindowMs ↑ |
| **Glitching** (끊김) | 경계 불연속 | overlapMs ↑ |
| **Chipmunk** (다람쥐 효과) | 포먼트 이동 | Formant preservation (미구현) |
| **Muffled** (먹먹함) | 고주파 손실 | Cubic interpolation |

---

## ⚡ 최적화 기법

### 1. SIMD (4-way unrolling)
```cpp
for (int i = 0; i < simdSize; i += 4) {
    sum += buf[i] + buf[i+1] + buf[i+2] + buf[i+3];
}
// 2-4배 빠름
```

### 2. 2단계 탐색
```
Coarse Search (2샘플 건너뛰기) → 50% 단축
Early Exit (corr > 0.95) → 추가 40% 단축
Fine Search (±2 samples) → 정밀도 유지

총: 5-8배 빠름
```

### 3. 메모리 풀링
```cpp
auto buffer = pool.acquire(size);  // 재사용
pool.release(std::move(buffer));   // 반환
// malloc/free 오버헤드 제거
```

### 4. 조기 종료
```cpp
if (std::abs(semitones) < 0.01f) return input;
// 처리 건너뛰기
```

---

## 🔧 핵심 함수 요약

### Time Stretcher
```cpp
// 메인 처리
AudioBuffer process(input, ratio)
  → 세그먼트 분할
  → findBestOverlapPosition() 반복
  → overlapAndAdd()

// 최적 위치 탐색
int findBestOverlapPosition(buf1, buf2)
  → Coarse Search (2샘플 간격)
  → Early Exit (corr > 0.95)
  → Fine Search (1샘플 간격)

// 유사도 계산
float calculateCorrelation(buf1, buf2)
  → Σ(buf1×buf2) / √(Σbuf1² × Σbuf2²)
  → SIMD 4-way 최적화

// 블렌딩
void overlapAndAdd(output, segment)
  → Linear crossfade (8ms)
```

### Pitch Shifter
```cpp
// 메인 처리
AudioBuffer process(input, semitones)
  → semitonesToRatio(semitones)
  → TimeStretcher.process(input, 1/ratio)
  → resample(stretched, ratio)

// 반음 변환
float semitonesToRatio(semitones)
  → 2^(semitones/12)

// 리샘플링
vector<float> resample(input, ratio)
  → SIMD 4-way 언롤링
  → linearInterpolate()
```

---

## 🎤 자주 묻는 질문 (Top 10)

### 1. 왜 WSOLA인가?
> 속도 (10-50ms) + WebAssembly 최적 + FFT 불필요

### 2. SoundTouch와 차이?
> 우리: 더 빠름 (2배), 코드 간단 (500줄 vs 5000줄)
> SoundTouch: 더 고품질, 프로덕션 검증

### 3. 실시간 가능?
> 네, 2초 오디오 10-50ms 처리 (40배 빠름)

### 4. 메모리 사용량?
> ~200KB (2초, 44.1kHz, 모노) + BufferPool 재사용

### 5. SIMD 어떻게?
> 4개씩 언롤링 → 컴파일러 자동 벡터화 → SSE/AVX

### 6. 가장 느린 부분?
> findBestOverlapPosition() (전체의 70-80%)
> → Coarse+Fine 탐색으로 5-8배 최적화

### 7. 아티팩트 원인?
> Time: 세그먼트 경계 불연속
> Pitch: 포먼트 이동 (Chipmunk effect)

### 8. 품질 개선 방법?
> seekWindowMs↑, overlapMs↑ (속도↓)
> 또는 Phase Vocoder 구현

### 9. 극단적 시프트 (±12 초과)?
> 품질 저하 심함
> → Phase Vocoder 필요

### 10. 다음 개선 계획?
> 1) Phase Vocoder (KISSFFT)
> 2) Formant preservation
> 3) Wasm SIMD explicit

---

## 📐 데이터 흐름

```
Float32Array (JS)
      ↓
AudioBuffer (C++)
      ↓
┌─────────────────┐
│ SimplePitchShifter
│   ↓
│ SimpleTimeStretcher (WSOLA)
│   ├─ Segment extraction (40ms)
│   ├─ Best position search (correlation)
│   └─ Overlap-add (crossfade)
│   ↓
│ Resampler (Linear interpolation)
└─────────────────┘
      ↓
AudioBuffer (output)
      ↓
typed_memory_view (Zero-copy)
      ↓
Float32Array (JS)
```

---

## 🧪 테스트 & 벤치마크

```bash
# 빌드
./tests/build_reconstruction_test.sh

# 실행
./tests/test_reconstruction

# 결과
tests/BENCHMARK_REPORT.md  (성능 비교)
profile.json               (함수별 시간)
```

---

## 💡 발표 팁

### 질문 대응
1. **파일 위치 언급**: "src/dsp/SimpleTimeStretcher.cpp 187줄에..."
2. **개념 먼저**: "두 파형이 얼마나 비슷한지... 이걸 correlation이라고..."
3. **예시 활용**: "+12 semitones는 피아노 한 옥타브..."
4. **한계 솔직히**: "±7 semitones에서 품질 좋고, 그 이상은..."
5. **수치 강조**: "2초 오디오를 10ms에 처리..."

### 시연 준비
- 브라우저 Live Demo
- PerformanceChecker JSON 출력
- 코드 빠른 네비게이션

---

## 🔗 유용한 링크

```
메인 가이드:     PRESENTATION_GUIDE.md (상세 30개 Q&A)
벤치마크:        tests/BENCHMARK_REPORT.md
소스 코드:       src/dsp/, src/audio/
테스트:          tests/test_*.cpp
외부 라이브러리:  src/external/soundtouch/, kissfft/
```

---

**이 한 장만 출력해서 발표 때 옆에 두세요! 📄✨**
