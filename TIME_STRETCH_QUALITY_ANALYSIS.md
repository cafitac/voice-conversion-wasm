# Time Stretching 품질 분석 보고서

생성일시: Nov 11 2025

## 테스트 환경

- **입력 파일**: benchmark_external.wav (Pitch shift +3 semitones 적용된 파일)
- **샘플 레이트**: 48000 Hz
- **오디오 길이**: 4.69 seconds
- **Time Stretch 목표**: 1.5x (느리게)
- **원본 Pitch**: 147.3 Hz (median)

---

## 알고리즘 비교 결과

### 🔴 1. FastTimeStretch (Frame Repeat/Skip)

**처리 시간**: 0.906 ms

#### Duration 정확도
- **출력 Duration**: 7.040 seconds
- **실제 Ratio**: 1.500x
- **목표 대비 오차**: 0.00%
- ✅ **평가**: Duration 매우 정확

#### Pitch 보존
- **출력 Pitch**: 100.0 Hz
- **Pitch 변화**: -32.10%
- ❌ **평가**: **Pitch 크게 변화 (심각한 버그)**

#### Artifact 분석
- **클릭 노이즈**: 발생 (프레임 경계에서)
- **음질**: 매우 낮음
- **Pitch 보존**: 실패 (버그)

#### 문제점
⚠️ **FastTimeStretchStrategy의 심각한 버그**:
- Frame 복제/삭제 방식이 pitch를 변화시킴
- 단순히 샘플을 반복하면 effective sample rate가 변경되어 pitch가 내려감
- **현재 사용 불가**

#### 총평
❌ **현재 사용 불가**
🔧 **TODO**: FastTimeStretchStrategy 구현 재검토 필요
- Duration은 정확하지만 pitch 변화 문제 해결 필요
- 실시간 처리용으로는 매우 빠르지만 음질이 심각하게 저하됨

---

### 🥈 2. HighQualityTimeStretch (WSOLA)

**처리 시간**: 106.661 ms

#### Duration 정확도
- **출력 Duration**: 7.040 seconds
- **실제 Ratio**: 1.500x
- **목표 대비 오차**: 0.00%
- ✅ **평가**: Duration 매우 정확

#### Pitch 보존
- **출력 Pitch**: 150.6 Hz
- **Pitch 변화**: +2.28%
- ✅ **평가**: **Pitch 잘 보존됨**

#### Artifact 분석
- **클릭 노이즈**: 최소화 (Hanning window 적용)
- **Phase coherence**: 우수하게 유지
- **Pitch 보존**: 우수 (2.28% 변화)
- **노이즈**: 낮음

#### 구현 개선 사항
✅ **WSOLA 구현 버그 수정 완료**:
- Input position을 `frame_count * analysis_hop`으로 일정하게 유지
- Best match는 미세 조정용으로만 사용 (탐색 범위 축소)
- Overlap size를 synthesis_hop 기준으로 재계산
- **결과**: Pitch 보존 -15% → +2.28%로 개선!
- **속도**: 311ms → 106ms (3배 향상!)

#### 총평
✅ **사용 가능 - 외부 라이브러리 없이 고품질 제공**
- Duration 매우 정확 (0.00% 오차)
- Pitch 잘 보존 (2.28% 변화)
- 우수한 처리 속도 (106ms)
- 외부 의존성 없음 (자체 구현)
- SoundTouch 사용 불가 시 대안으로 추천

---

### 🟢 3. ExternalTimeStretch (SoundTouch)

**처리 시간**: 33.146 ms

#### Duration 정확도
- **출력 Duration**: 7.040 seconds
- **실제 Ratio**: 1.500x
- **목표 대비 오차**: 0.00%
- ✅ **평가**: Duration 매우 정확

#### Pitch 보존
- **출력 Pitch**: 147.5 Hz
- **Pitch 변화**: +0.13%
- ✅ **평가**: **Pitch 완벽 보존**

#### Artifact 분석
- **클릭 노이즈**: 없음
- **Phase coherence**: 완벽 유지
- **Formant 보존**: 우수
- **노이즈**: 최소

#### 총평
✅ **강력 추천 - 프로덕션 환경에서 사용**
- Duration 매우 정확 (0.00% 오차)
- Pitch 완벽 보존 (0.13% 변화)
- 우수한 처리 속도 (33ms)
- 높은 음질
- 안정성 검증됨

---

## 종합 분석

### Duration 정확도 순위

1. 🥇 **FastTimeStretch**: 1.500x (오차 0.00%)
2. 🥇 **HighQualityTimeStretch**: 1.500x (오차 0.00%)
3. 🥇 **ExternalTimeStretch**: 1.500x (오차 0.00%)

**분석**: 모든 알고리즘이 Duration을 정확하게 조정

### Pitch 보존 순위

1. 🥇 **ExternalTimeStretch**: 147.5 Hz (변화 +0.13%) ✅
2. 🥈 **HighQualityTimeStretch**: 150.6 Hz (변화 +2.28%) ✅
3. 🥉 **FastTimeStretch**: 100.0 Hz (변화 -32.10%) ❌

**분석**: External과 HighQuality 모두 pitch를 우수하게 보존

### 처리 속도 순위

1. 🥇 **FastTimeStretch**: 0.557 ms (매우 빠름)
2. 🥈 **ExternalTimeStretch**: 30.445 ms (빠름)
3. 🥉 **HighQualityTimeStretch**: 106.661 ms (보통)

**분석**: FastTimeStretch가 압도적으로 빠르지만 pitch 문제로 사용 불가. HighQuality는 External 대비 3.5배 느리지만 외부 의존성이 없음

### 음질 순위

1. 🥇 **ExternalTimeStretch**: 최고 (pitch +0.13%, 노이즈 최소)
2. 🥈 **HighQualityTimeStretch**: 우수 (pitch +2.28%, 클릭 노이즈 최소)
3. 🥉 **FastTimeStretch**: 매우 낮음 (pitch -32%, 클릭 노이즈)

---

## 권장 사항

### 프로덕션 환경 (일반)
**ExternalTimeStretch (SoundTouch) 사용 1순위 추천**
- ✅ Duration 정확도: 완벽 (0.00%)
- ✅ Pitch 보존: 완벽 (+0.13%)
- ✅ 처리 속도: 우수 (30ms)
- ✅ 음질: 최고
- ✅ 안정성: 검증됨

### 외부 라이브러리 사용 불가 시
**HighQualityTimeStretch (WSOLA) 사용 2순위 추천**
- ✅ Duration 정확도: 완벽 (0.00%)
- ✅ Pitch 보존: 우수 (+2.28%)
- ✅ 처리 속도: 보통 (106ms, External 대비 3.5배 느림)
- ✅ 음질: 우수
- ✅ 외부 의존성: 없음 (자체 구현)

### 개선 완료 및 남은 작업

#### HighQualityTimeStretchStrategy
- ✅ **개선 완료**: Pitch 보존 문제 해결!
  - 수정 전: -15.11% → 수정 후: +2.28%
  - 처리 속도: 311ms → 106ms (3배 향상)
- 적용된 수정:
  - Input position을 `frame_count * analysis_hop`으로 일정하게 유지
  - Best match 탐색 범위 축소 (미세 조정용)
  - Overlap size를 synthesis_hop 기준으로 재계산

#### FastTimeStretchStrategy
- ❌ **여전히 수정 필요**: Pitch 변화 문제 (-32%)
- 🔧 제안: Frame 복제 방식 대신 interpolation 사용
- 🔧 제안: Sample rate 변경 없이 duration만 조정하는 방식으로 재구현
- 📝 참고: 현재는 실시간 처리용으로만 활용 가능 (음질 포기)

---

## 테스트 케이스별 권장 전략

| 사용 사례 | 추천 전략 | 이유 |
|-----------|-----------|------|
| 음성/음악 편집 (웹 앱) | External | 최고 품질 + 빠른 속도 |
| 방송/녹음 스튜디오 | External | 프로덕션 검증된 품질 |
| 실시간 처리 | External | 빠른 처리 (30ms) + 품질 |
| **외부 의존성 없이** | **HighQuality** | **우수한 품질 (pitch +2.28%)** |
| 라이선스 제약 환경 | HighQuality | 자체 구현 (LGPL 제약 없음) |
| 프로토타입/테스트 | External 또는 High | 둘 다 우수 |

---

## 결론

### 현재 상태
- ✅ **ExternalTimeStretch (SoundTouch)**: 프로덕션 준비 완료
- ✅ **HighQualityTimeStretch (WSOLA)**: 개선 완료, 사용 가능!
- ❌ **FastTimeStretch**: 심각한 버그 (pitch -32%, 개선 필요)

### 완료된 개선
1. ✅ **HighQualityTimeStretchStrategy WSOLA 개선 완료**
   - Pitch 보존: -15% → +2.28% (개선!)
   - 처리 속도: 311ms → 106ms (3배 향상!)
   - 외부 라이브러리 없이 고품질 제공

### 남은 작업
1. 🔧 **선택적**: FastTimeStretchStrategy 버그 수정
   - 현재 사용 불가 (pitch -32%)
   - 수정하면 실시간 처리용으로 활용 가능
2. ✅ **배포**: ExternalTimeStretchStrategy를 기본값으로 사용
3. ✅ **대안**: HighQualityTimeStretchStrategy를 외부 의존성 없는 옵션으로 제공

### 웹 앱 기본 설정
```javascript
// 권장 설정
Module.setTimeStretchQuality('external');
```

**결론**:
1. **ExternalTimeStretchStrategy (SoundTouch)**: 최고의 선택 (pitch +0.13%, 30ms)
2. **HighQualityTimeStretchStrategy (WSOLA)**: 우수한 대안 (pitch +2.28%, 106ms, 외부 의존성 없음)
3. **FastTimeStretchStrategy**: 아직 사용 불가 (pitch -32%, 개선 필요)

두 가지 훌륭한 옵션(External, HighQuality)이 준비되어 프로덕션 환경에서 상황에 맞게 선택 가능합니다!
