# 개발 규칙 (Development Rules)

이 문서는 본 프로젝트의 핵심 설계 원칙과 개발 프로세스를 정의합니다.

---

## 1. Strategy Pattern 설계 원칙

### 1.1 Pitch Shift Strategies

| 전략 | 구현 방식 | 외부 라이브러리 | 특징 |
|------|-----------|----------------|------|
| **FastPitchShiftStrategy** | Simple Resampling (간단한 리샘플링) | ❌ 사용 안 함 | 매우 빠른 처리, 낮은 품질 |
| **HighQualityPitchShiftStrategy** | Phase Vocoder (위상 보코더) | ❌ 사용 안 함 | 자체 알고리즘 구현, 고품질 |
| **ExternalPitchShiftStrategy** | SoundTouch 라이브러리 | ✅ SoundTouch | 검증된 프로덕션 품질 |

**규칙**:
- Fast는 **속도 최우선**, 알고리즘 복잡도 최소화
- High는 **자체 구현 알고리즘**, 외부 의존성 없이 고품질 제공
- External은 **외부 라이브러리만 사용**, 검증된 품질

### 1.2 Time Stretch Strategies

| 전략 | 구현 방식 | 외부 라이브러리 | 특징 |
|------|-----------|----------------|------|
| **FastTimeStretchStrategy** | Frame 복제/삭제 (단순 프레임 조작) | ❌ 사용 안 함 | 매우 빠른 처리, 낮은 품질 |
| **HighQualityTimeStretchStrategy** | WSOLA 알고리즘 | ❌ 사용 안 함 | 자체 알고리즘 구현, 고품질 |
| **ExternalTimeStretchStrategy** | SoundTouch 라이브러리 | ✅ SoundTouch | 검증된 프로덕션 품질 |

**규칙**:
- Fast는 **frame 단위 작업 기반**, 복잡한 알고리즘 사용 금지
- High는 **WSOLA 등 알고리즘 구현**, 외부 라이브러리 사용 금지
- External은 **SoundTouch만 사용**, 자체 알고리즘 구현 금지

---

## 2. 코드 수정 프로세스

### 2.1 필수 단계

모든 코드 수정은 다음 순서를 **반드시** 따라야 합니다:

```
1. 수정 전 성능 측정 (벤치마크 실행)
   ↓
2. 코드 수정
   ↓
3. 빌드 및 컴파일
   ↓
4. 벤치마크 재실행
   ↓
5. 결과 비교 및 분석
   ↓
6. 성능 개선 확인
   ├─ ✅ 개선됨 → 보고서 업데이트 → 커밋
   └─ ❌ 미개선/악화 → 코드 롤백 → 다른 방법 시도
```

### 2.2 성능 개선 기준

#### Pitch Shift 전략
- **Pitch 정확도**: 목표 ±0.5 semitones 이내
- **Duration 유지**: 원본 대비 ±1% 이내
- **처리 속도**:
  - Fast: 1ms 이내
  - High: 100ms 이내
  - External: 50ms 이내

#### Time Stretch 전략
- **Duration 정확도**: 목표 ±1% 이내
- **Pitch 보존**: 원본 대비 ±5% 이내
- **처리 속도**:
  - Fast: 1ms 이내
  - High: 150ms 이내
  - External: 50ms 이내

### 2.3 보고서 업데이트 규칙

**중요**: 전략 수정 시 관련된 **모든** 보고서를 업데이트해야 합니다.

#### PitchShift 전략 수정 시
1. `QUALITY_ANALYSIS.md` 업데이트 (필수)
2. `COMBINED_QUALITY_ANALYSIS.md` 업데이트 (필수)
   - Combined 벤치마크 재실행 필요

#### TimeStretch 전략 수정 시
1. `TIME_STRETCH_QUALITY_ANALYSIS.md` 업데이트 (필수)
2. `COMBINED_QUALITY_ANALYSIS.md` 업데이트 (필수)
   - Combined 벤치마크 재실행 필요

**이유**: Combined 처리는 두 전략을 함께 사용하므로, 한쪽이 개선되면 Combined 결과도 변경됩니다.

### 2.4 롤백 기준

다음 경우 **즉시 롤백**:
- ❌ Pitch/Duration 정확도가 악화된 경우
- ❌ 처리 속도가 2배 이상 느려진 경우
- ❌ 새로운 버그 발생 (크래시, 메모리 누수 등)
- ❌ 음질이 명확하게 저하된 경우
- ❌ 관련 보고서를 업데이트하지 않은 경우

---

## 3. 벤치마크 및 품질 분석

### 3.1 벤치마크 파일

| 벤치마크 | 파일 경로 | 용도 |
|----------|-----------|------|
| Pitch Shift | `tests/benchmark_pitchshift.cpp` | Pitch shift 전략 비교 |
| Time Stretch | `tests/benchmark_timestretch.cpp` | Time stretch 전략 비교 |
| Combined | `tests/benchmark_combined.cpp` | Pitch + Duration 결합 비교 |

### 3.2 품질 분석 문서

| 문서 | 파일 경로 | 내용 |
|------|-----------|------|
| Pitch Shift 분석 | `QUALITY_ANALYSIS.md` | Pitch shift 전략 품질 분석 |
| Time Stretch 분석 | `TIME_STRETCH_QUALITY_ANALYSIS.md` | Time stretch 전략 품질 분석 |
| Combined 분석 | `COMBINED_QUALITY_ANALYSIS.md` | 결합 처리 품질 분석 |

### 3.3 보고서 업데이트 규칙

코드 수정 후 **반드시** 다음을 수행:

1. **벤치마크 재실행**
   ```bash
   cd tests
   ./build_benchmark_timestretch.sh  # 또는 해당 벤치마크
   ./benchmark_timestretch benchmark_external.wav 1.5
   ```

2. **결과 비교**
   - 수정 전 vs 수정 후 성능 비교
   - Pitch/Duration 정확도 확인
   - 처리 속도 확인

3. **보고서 업데이트**
   - 측정된 값으로 문서 업데이트
   - 개선/악화 사항 명시
   - 롤백 여부 결정

4. **Git Commit**
   - 개선 시: "feat: Improve [전략명] - [개선 내용]"
   - 롤백 시: "revert: Rollback [전략명] - [이유]"

---

## 4. 파일 구조 규칙

### 4.1 효과(Effects) 파일 구조

```
src/effects/
├── pitchshift/          # Pitch shift 관련 (향후 분리 예정)
│   ├── IPitchShiftStrategy.h
│   ├── FastPitchShiftStrategy.{h,cpp}
│   ├── HighQualityPitchShiftStrategy.{h,cpp}
│   └── ExternalPitchShiftStrategy.{h,cpp}
│
├── timestretch/         # Time stretch 관련 (향후 분리 예정)
│   ├── ITimeStretchStrategy.h
│   ├── FastTimeStretchStrategy.{h,cpp}
│   ├── HighQualityTimeStretchStrategy.{h,cpp}
│   └── ExternalTimeStretchStrategy.{h,cpp}
│
└── [기타 효과 파일들...]
```

**현재 상태**: 모든 파일이 `src/effects/` 하위에 있음
**향후 계획**: pitchshift/, timestretch/ 서브폴더로 분리

---

## 5. 구현 가이드라인

### 5.1 FastTimeStretchStrategy 구현 원칙

✅ **허용**:
- Frame 복제/삭제를 통한 duration 조정
- 단순 선형 보간
- 기본적인 windowing (Hanning, Hamming)

❌ **금지**:
- WSOLA, Phase Vocoder 같은 복잡한 알고리즘
- 외부 라이브러리 사용
- 100줄 이상의 복잡한 로직

**목표**: 1ms 이내 처리 속도, 단순한 구현

### 5.2 HighQualityTimeStretchStrategy 구현 원칙

✅ **허용**:
- WSOLA (Waveform Similarity Overlap-Add)
- Phase Vocoder
- OLA (Overlap-Add) 변형 알고리즘
- Best match 탐색 (cross-correlation)
- Hanning/Hamming window

❌ **금지**:
- 외부 라이브러리 사용 (SoundTouch, RubberBand 등)
- 300줄 이상의 단일 함수

**목표**: 외부 의존성 없이 고품질 제공, 150ms 이내 처리

### 5.3 ExternalTimeStretchStrategy 구현 원칙

✅ **허용**:
- SoundTouch 라이브러리 API 호출
- SoundTouch 파라미터 설정
- 버퍼 관리 및 샘플 전달

❌ **금지**:
- 자체 알고리즘 구현
- 다른 외부 라이브러리 사용
- SoundTouch 내부 로직 수정

**목표**: 검증된 라이브러리의 최고 품질 활용

---

## 6. 테스트 입력 파일

### 6.1 표준 테스트 파일

| 파일명 | 용도 | 특징 |
|--------|------|------|
| `original.wav` | 원본 오디오 | 녹음된 원본 파일 |
| `benchmark_fast.wav` | Fast 전략 테스트 | Fast 전략 출력 |
| `benchmark_high.wav` | High 전략 테스트 | High 전략 출력 |
| `benchmark_external.wav` | External 전략 테스트 | External 전략 출력 |

### 6.2 테스트 파라미터

**Pitch Shift**:
- 기본값: +3 semitones
- 범위: -12 ~ +12 semitones

**Time Stretch**:
- 기본값: 1.5x (느리게)
- 범위: 0.5x ~ 2.0x

**Combined**:
- Pitch: +3 semitones
- Duration: 1.5x

---

## 7. 코드 리뷰 체크리스트

코드 수정 시 다음을 확인:

### 7.1 전략 준수
- [ ] Fast 전략에 외부 라이브러리를 사용하지 않았는가?
- [ ] High 전략에 외부 라이브러리를 사용하지 않았는가?
- [ ] External 전략에 자체 알고리즘을 구현하지 않았는가?

### 7.2 성능 검증
- [ ] 벤치마크를 실행했는가?
- [ ] 성능이 개선되었는가?
- [ ] 보고서를 업데이트했는가?
- [ ] 롤백이 필요하지 않은가?

### 7.3 코드 품질
- [ ] 메모리 누수가 없는가?
- [ ] 경계 조건을 처리했는가?
- [ ] 주석이 충분한가?
- [ ] 변수명이 명확한가?

---

## 8. 예시: 코드 수정 워크플로우

### 8.1 HighQualityTimeStretchStrategy 개선 예시

```bash
# 1. 수정 전 벤치마크
cd tests
./benchmark_timestretch benchmark_external.wav 1.5 > before.txt

# 수정 전 결과:
# - Pitch 변화: -15.11%
# - 처리 시간: 311ms

# 2. 코드 수정
# (HighQualityTimeStretchStrategy.cpp 수정)

# 3. 빌드
./build_benchmark_timestretch.sh

# 4. 수정 후 벤치마크
./benchmark_timestretch benchmark_external.wav 1.5 > after.txt

# 수정 후 결과:
# - Pitch 변화: +2.28%
# - 처리 시간: 106ms

# 5. 결과 비교
diff before.txt after.txt

# 6. 성능 개선 확인
# ✅ Pitch 보존: -15.11% → +2.28% (개선!)
# ✅ 속도: 311ms → 106ms (3배 향상!)

# 7. 보고서 업데이트
# TIME_STRETCH_QUALITY_ANALYSIS.md 업데이트

# 8. Commit
git add .
git commit -m "feat: Improve HighQualityTimeStretch WSOLA - Fix pitch preservation"
```

### 8.2 롤백 예시

```bash
# 1. 수정 전 벤치마크
./benchmark_timestretch benchmark_external.wav 1.5 > before.txt

# 2. 코드 수정 후 벤치마크
./benchmark_timestretch benchmark_external.wav 1.5 > after.txt

# 3. 결과 비교
# ❌ Pitch 변화: +2% → +15% (악화!)
# ❌ 속도: 100ms → 300ms (3배 느려짐!)

# 4. 즉시 롤백
git checkout HEAD -- src/effects/HighQualityTimeStretchStrategy.cpp

# 5. 다른 접근 방법 시도
# ...
```

---

## 9. 금지 사항

### 9.1 절대 금지
- ❌ 벤치마크 없이 코드 수정 후 커밋
- ❌ Fast 전략에 외부 라이브러리 추가
- ❌ High 전략에 외부 라이브러리 추가
- ❌ External 전략을 자체 알고리즘으로 교체
- ❌ 성능 악화를 그대로 커밋
- ❌ 보고서 업데이트 없이 커밋
- ❌ **PitchShift 수정 후 COMBINED 보고서 미업데이트**
- ❌ **TimeStretch 수정 후 COMBINED 보고서 미업데이트**

### 9.2 주의 필요
- ⚠️ 메모리 할당 최적화 (누수 주의)
- ⚠️ 버퍼 크기 변경 (경계 확인)
- ⚠️ 알고리즘 파라미터 변경 (품질 영향)

---

## 10. 버전 히스토리

| 날짜 | 버전 | 변경 내용 |
|------|------|-----------|
| 2025-11-11 | 1.0 | 초기 문서 작성 |

---

**이 규칙을 위반하는 모든 코드 변경은 즉시 롤백됩니다.**
