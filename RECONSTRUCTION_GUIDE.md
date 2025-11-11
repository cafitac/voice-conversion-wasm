# FrameData → AudioBuffer 재구성 가이드

## 개요

이 기능은 분석된 오디오의 Pitch와 Duration을 수정한 후, 수정된 데이터를 다시 오디오 파일로 변환할 수 있게 해줍니다.

## 핵심 구조

### 1. AudioPreprocessor 새로운 메서드

```cpp
// FrameData를 AudioBuffer로 재구성 (Overlap-Add 합성)
AudioBuffer reconstruct(
    const std::vector<FrameData>& frames,
    int sampleRate,
    int channels,
    float hopSize
);

// 프레임별 Pitch 수정 적용
void applyPitchModification(
    std::vector<FrameData>& frames,
    const std::vector<float>& pitchShifts,  // semitones
    int sampleRate
);

// 프레임별 Duration 수정 적용
void applyDurationModification(
    std::vector<FrameData>& frames,
    const std::vector<float>& timeRatios,  // 1.0 = 원본, 1.2 = 20% 늘림
    int sampleRate
);
```

### 2. JavaScript API

```javascript
// 오디오 수정 및 재구성
const modifiedAudio = Module.modifyAndReconstructAudio(
    audioDataPtr,           // Float32Array pointer
    audioLength,            // 샘플 수
    sampleRate,             // 48000
    pitchShiftsPtr,         // Float32Array (각 프레임의 semitones)
    pitchShiftsLength,      // 프레임 수
    timeRatiosPtr,          // Float32Array (각 프레임의 time ratio)
    timeRatiosLength        // 프레임 수
);

// 재구성만 (수정 없이)
const reconstructed = Module.reconstructAudioFromFrames(
    audioDataPtr,
    audioLength,
    sampleRate
);

// WAV 파일로 변환
const wavData = Module.getModifiedAudioAsWav(
    modifiedAudioPtr,
    modifiedLength,
    sampleRate
);
```

## 사용 예시

### C++ (Native)

```cpp
#include "src/audio/AudioBuffer.h"
#include "src/audio/AudioPreprocessor.h"

// 1. 오디오 로드
AudioBuffer original = loadWavFile("input.wav");

// 2. 전처리
AudioPreprocessor preprocessor;
auto frames = preprocessor.process(original, 0.02f, 0.01f, 0.02f);

// 3. Pitch 수정 (모든 프레임을 +2 semitones)
std::vector<float> pitchShifts(frames.size(), 2.0f);
preprocessor.applyPitchModification(frames, pitchShifts, 48000);

// 4. Duration 수정 (1.2배 늘림)
std::vector<float> timeRatios(frames.size(), 1.2f);
preprocessor.applyDurationModification(frames, timeRatios, 48000);

// 5. 재구성
AudioBuffer result = preprocessor.reconstruct(frames, 48000, 1, 0.01f);

// 6. 저장
saveWavFile("output.wav", result);
```

### JavaScript (WebAssembly)

```javascript
// 오디오 녹음/업로드 후
const audioContext = new AudioContext();
const audioBuffer = ...; // 오디오 데이터

// Float32Array로 변환
const audioData = audioBuffer.getChannelData(0);

// WASM 메모리에 복사
const dataPtr = Module._malloc(audioData.length * 4);
Module.HEAPF32.set(audioData, dataPtr / 4);

// Pitch 조정 배열 생성 (예: 각 프레임마다 다른 값)
const pitchShifts = new Float32Array(468); // 프레임 수
for (let i = 0; i < pitchShifts.length; i++) {
    // 시간에 따라 0 → +5 semitones로 변화
    pitchShifts[i] = (i / pitchShifts.length) * 5.0;
}

const pitchPtr = Module._malloc(pitchShifts.length * 4);
Module.HEAPF32.set(pitchShifts, pitchPtr / 4);

// Duration 조정 배열 생성
const timeRatios = new Float32Array(468);
timeRatios.fill(1.0); // 모두 원본 속도

const ratioPtr = Module._malloc(timeRatios.length * 4);
Module.HEAPF32.set(timeRatios, ratioPtr / 4);

// 수정 및 재구성
const result = Module.modifyAndReconstructAudio(
    dataPtr,
    audioData.length,
    48000,
    pitchPtr,
    pitchShifts.length,
    ratioPtr,
    timeRatios.length
);

// WAV 변환 및 다운로드
const wavData = Module.getModifiedAudioAsWav(
    result.byteOffset,
    result.length,
    48000
);

// Blob 생성 및 다운로드
const blob = new Blob([wavData], { type: 'audio/wav' });
const url = URL.createObjectURL(blob);
const a = document.createElement('a');
a.href = url;
a.download = 'modified.wav';
a.click();

// 메모리 해제
Module._free(dataPtr);
Module._free(pitchPtr);
Module._free(ratioPtr);
```

## 알고리즘 상세

### Overlap-Add 합성

프레임을 다시 연속된 오디오로 합성할 때 사용하는 알고리즘입니다.

```
frameSize = 20ms (960 samples at 48kHz)
hopSize = 10ms (480 samples) → 50% overlap

Frame 0: [0-20ms]    ▁▂▃▄▅▆▇█▇▆▅▄▃▂▁
Frame 1:     [10-30ms]   ▁▂▃▄▅▆▇█▇▆▅▄▃▂▁
Frame 2:         [20-40ms]   ▁▂▃▄▅▆▇█▇▆▅▄▃▂▁

Overlap 구간에서 Hanning window로 가중치 부여:
output[i] = Σ (frame[j][k] * window[k])
```

**Hanning Window 공식:**
```
w(n) = 0.5 * (1 - cos(2π * n / (N-1)))
```

이를 통해:
- 프레임 경계에서 클릭 노이즈 방지
- 부드러운 연결
- 원본 신호의 충실한 복원

### 품질 검증

테스트 결과:
```
원본 → 전처리 → 재구성 (수정 없이)
RMS 오차: 0.000000

→ 완벽한 재구성 (무손실)
```

## 워크플로우

```
1. 사용자가 오디오 녹음/업로드
   ↓
2. AudioPreprocessor.process() → FrameData 생성
   ↓
3. PitchAnalyzer, DurationAnalyzer로 분석
   ↓
4. 사용자가 UI에서 그래프 조작
   (예: pitch 곡선을 마우스로 그림)
   ↓
5. 수정된 값을 Float32Array로 준비
   ↓
6. modifyAndReconstructAudio() 호출
   ↓
7. 결과를 WAV로 변환
   ↓
8. 다운로드
```

## 테스트

```bash
# 네이티브 테스트 빌드
./build_reconstruction_test.sh

# 실행
./test_reconstruction

# 결과 확인
ls -lh *.wav
# reconstructed.wav - 재구성 품질 확인
# pitch_shifted.wav - +2 semitones
# time_stretched.wav - 1.2배 늘림
# combined_modified.wav - 복합 수정
```

## 주의사항

1. **메모리 관리**: JavaScript에서 `Module._malloc()` 후 반드시 `Module._free()` 호출
2. **프레임 수 계산**: `frameSize=20ms, hopSize=10ms`일 때 `프레임 수 ≈ (duration / hopSize)`
3. **순서**: Duration 먼저 적용 후 Pitch 적용 권장
4. **배열 크기**: pitchShifts, timeRatios는 프레임 수와 같거나 작아야 함 (작으면 마지막 값 재사용)

## 향후 개선 사항

- [ ] 실시간 프리뷰 기능
- [ ] 구간별 다른 효과 적용 (trim과 연동)
- [ ] GPU 가속 (WebGL)
- [ ] 더 고급 알고리즘 (Phase Vocoder)
