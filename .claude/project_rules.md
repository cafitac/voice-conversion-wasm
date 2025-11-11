# 프로젝트 규칙 (Project Rules for Claude)

Claude는 이 프로젝트에서 코드 수정 시 **반드시** 다음 규칙을 따라야 합니다.

---

## 🔴 핵심 규칙 (CRITICAL RULES)

### 1. Strategy Pattern 설계 원칙

#### TimeStretch 전략 구분:
- **Fast**: Frame 복제/삭제 **만** 사용 (외부 라이브러리 금지)
- **HighQuality**: WSOLA 등 알고리즘 구현 (외부 라이브러리 금지)
- **External**: SoundTouch 라이브러리 **만** 사용 (자체 구현 금지)

#### PitchShift 전략 구분:
- **Fast**: 간단한 리샘플링 **만** 사용 (외부 라이브러리 금지)
- **HighQuality**: Phase Vocoder 구현 (외부 라이브러리 금지)
- **External**: SoundTouch 라이브러리 **만** 사용 (자체 구현 금지)

### 2. 코드 수정 필수 프로세스

```
수정 전 벤치마크 → 코드 수정 → 빌드 → 벤치마크 → 결과 비교
  ↓
✅ 개선됨 → 보고서 업데이트 → 커밋
❌ 미개선 → 즉시 롤백 → 다른 방법 시도
```

**벤치마크 없이 커밋 절대 금지!**

---

## 🚫 절대 금지 사항

1. ❌ **Fast/High 전략에 외부 라이브러리 사용**
2. ❌ **External 전략에 자체 알고리즘 구현**
3. ❌ **벤치마크 없이 코드 수정 후 커밋**
4. ❌ **성능 악화를 그대로 커밋**
5. ❌ **보고서 업데이트 없이 커밋**

---

## 📋 벤치마크 명령어

```bash
# Time Stretch 벤치마크
cd tests
./build_benchmark_timestretch.sh
./benchmark_timestretch benchmark_external.wav 1.5

# Pitch Shift 벤치마크
./build_benchmark_pitchshift.sh
./benchmark_pitchshift original.wav 3

# Combined 벤치마크
./build_benchmark_combined.sh
./benchmark_combined benchmark_fast.wav 3.0 1.5
```

---

## 📊 성능 기준

### TimeStretch:
- **Duration 정확도**: ±1% 이내
- **Pitch 보존**: ±5% 이내
- **처리 속도**:
  - Fast: < 1ms
  - High: < 150ms
  - External: < 50ms

### PitchShift:
- **Pitch 정확도**: ±0.5 semitones
- **Duration 유지**: ±1% 이내
- **처리 속도**:
  - Fast: < 1ms
  - High: < 100ms
  - External: < 50ms

---

## 🔄 롤백 기준

다음 경우 **즉시 롤백**:
- Pitch/Duration 정확도 악화
- 처리 속도 2배 이상 느려짐
- 새로운 버그 발생
- 음질 저하

---

## 📝 보고서 업데이트 규칙

**중요**: 전략 수정 시 관련된 **모든** 보고서를 업데이트!

### PitchShift 수정 시:
1. ✅ `QUALITY_ANALYSIS.md`
2. ✅ `COMBINED_QUALITY_ANALYSIS.md` (Combined 벤치마크 재실행)

### TimeStretch 수정 시:
1. ✅ `TIME_STRETCH_QUALITY_ANALYSIS.md`
2. ✅ `COMBINED_QUALITY_ANALYSIS.md` (Combined 벤치마크 재실행)

**이유**: Combined는 두 전략을 함께 사용하므로 한쪽 개선 시 결과 변경됨

---

## ✅ 코드 수정 체크리스트

코드 수정 전 확인:
- [ ] 올바른 전략에 수정하는가? (Fast/High/External 구분)
- [ ] 외부 라이브러리 사용 규칙을 지키는가?
- [ ] 수정 전 벤치마크를 실행했는가?

코드 수정 후 확인:
- [ ] 빌드가 성공했는가?
- [ ] 벤치마크를 재실행했는가?
- [ ] 성능이 개선되었는가?
- [ ] 보고서를 업데이트했는가?

---

**자세한 내용은 `DEVELOPMENT_RULES.md` 참조**
