# Pitch Shifting 품질 분석 보고서

생성일시: Nov 11 2025 13:00:00

## 테스트 환경

- **입력 파일**: original.wav
- **샘플 레이트**: 48000 Hz
- **오디오 길이**: 4.69 seconds
- **Pitch Shift 목표**: +3 semitones
- **원본 Pitch**: 124.4 Hz (median)

---

## 알고리즘 비교 결과

### 🔴 1. FastPitchShift (Simple Resampling)

**처리 시간**: 0.335 ms

#### Pitch 정확도
- **출력 Pitch**: 147.4 Hz
- **실제 Shift**: 2.94 semitones
- **목표 대비 오차**: -0.06 semitones (-2.1%)
- ✅ **평가**: 매우 정확

#### Duration 유지
- **Duration Ratio**: 0.841 (84.1%)
- ❌ **평가**: Duration이 15.9% 줄어듦 (속도가 빨라짐)

#### Artifact 분석
- **Aliasing**: 발생 (고주파 노이즈)
- **Formant 왜곡**: 발생 (Chipmunk effect)
- **Phase coherence**: 없음

#### 총평
✅ **장점**: 극도로 빠름, pitch는 정확
❌ **단점**: Duration 변화, 음질 저하
**사용 권장**: 실시간 처리가 절대적으로 필요한 경우만

---

### 🟢 2. ExternalPitchShift (SoundTouch)

**처리 시간**: 26.994 ms

#### Pitch 정확도
- **출력 Pitch**: 147.3 Hz
- **실제 Shift**: 2.92 semitones
- **목표 대비 오차**: -0.08 semitones (-2.8%)
- ✅ **평가**: 매우 정확

#### Duration 유지
- **Duration Ratio**: 1.000 (100%)
- ✅ **평가**: Duration 완벽 유지

#### Artifact 분석
- **Aliasing**: 최소화 (anti-aliasing filter 적용)
- **Formant 보존**: 우수
- **Phase coherence**: 유지
- **노이즈**: 최소

#### 총평
✅ **장점**: 정확한 pitch, duration 유지, 우수한 음질, 안정성
✅ **사용 권장**: **웹 앱 기본값으로 강력 추천**

---

### 🟡 3. HighQualityPitchShift (Phase Vocoder - 직접 구현)

**처리 시간**: 57.734 ms

#### Pitch 정확도
- **출력 Pitch**: 347.3 Hz
- **실제 Shift**: 17.77 semitones
- **목표 대비 오차**: +14.77 semitones (+492.3%)
- ❌ **평가**: **심각한 버그 - 사용 불가**

#### Duration 유지
- **Duration Ratio**: 1.000 (100%)
- ✅ **평가**: Duration은 유지됨

#### 알려진 문제
⚠️ **현재 구현에 버그가 있어 pitch가 과도하게 올라갑니다.**
- Frequency bin mapping 로직 오류 의심
- Formant preservation 코드 오류 의심
- 추가 디버깅 및 수정 필요

#### 총평
❌ **현재 사용 불가**
🔧 **TODO**: Phase Vocoder 구현 재검토 필요

---

## 종합 분석

### Pitch 정확도 순위

1. 🥇 **FastPitchShift**: 2.94 semitones (오차 -2.1%)
2. 🥈 **ExternalPitchShift**: 2.92 semitones (오차 -2.8%)
3. 🥉 ~~HighQualityPitchShift~~: 버그로 인해 제외

### Duration 유지 순위

1. 🥇 **ExternalPitchShift**: 1.000 (100%)
2. 🥇 ~~HighQualityPitchShift~~: 1.000 (버그로 인해 제외)
3. 🥉 **FastPitchShift**: 0.841 (84.1%)

### 처리 속도 순위

1. 🥇 **FastPitchShift**: 0.335 ms (1.0x)
2. 🥈 **ExternalPitchShift**: 26.994 ms (80.6x slower)
3. 🥉 ~~HighQualityPitchShift~~: 57.734 ms (172.3x slower)

---

## 권장사항

### 현재 권장 알고리즘

| 사용 목적 | 권장 알고리즘 | 이유 |
|----------|--------------|------|
| **일반 웹 앱** | **ExternalPitchShift** | 정확도, duration 유지, 안정성 |
| **프로덕션** | **ExternalPitchShift** | 검증된 라이브러리, 업계 표준 |
| **실시간 게임/스트리밍** | FastPitchShift | 극도로 빠름 (단, duration 변화 허용) |
| **고품질 오디오 편집** | **ExternalPitchShift** | 현재 유일한 고품질 옵션 |

### JavaScript 사용법

```javascript
// 권장 (기본값)
Module.setPitchShiftQuality("external");  // SoundTouch

// 실시간 처리가 필요한 경우
Module.setPitchShiftQuality("fast");

// High Quality는 버그로 인해 현재 비권장
// Module.setPitchShiftQuality("high");  // 사용 금지
```

---

## 청취 테스트 파일

생성된 WAV 파일:
- `tests/benchmark_fast.wav` - FastPitchShift 결과
- `tests/benchmark_external.wav` - ExternalPitchShift 결과 (권장)
- `tests/benchmark_high.wav` - HighQualityPitchShift 결과 (버그 있음)

---

## 결론

**ExternalPitchShift (SoundTouch)**가 현재 **유일하게 사용 가능한 고품질 알고리즘**입니다.

- ✅ Pitch 정확도: 매우 우수 (-2.8% 오차)
- ✅ Duration 유지: 완벽
- ✅ 처리 속도: 충분히 빠름 (27ms)
- ✅ 음질: 프로덕션급
- ✅ 안정성: 업계 표준 라이브러리

**HighQualityPitchShift는 버그 수정 후 재평가 필요합니다.**
