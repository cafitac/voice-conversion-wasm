/**
 * Audio Utility Functions
 *
 * Pipeline 처리 결과 변환 및 기타 유틸리티 함수들
 */

/**
 * Pipeline 처리 결과를 Float32Array로 변환
 *
 * Module.processAudioWithPipeline()이 반환하는 결과 객체를
 * 표준 Float32Array로 변환합니다.
 *
 * @param {Object} resultView - Pipeline 처리 결과 (Module에서 반환)
 * @returns {Float32Array} 변환된 오디오 데이터
 *
 * @example
 * const resultView = Module.processAudioWithPipeline(...);
 * const audioData = convertPipelineResultToFloat32Array(resultView);
 */
export function convertPipelineResultToFloat32Array(resultView) {
    const result = new Float32Array(resultView.length);
    for (let i = 0; i < resultView.length; i++) {
        result[i] = resultView[i];
    }
    return result;
}

/**
 * WAV 형식 데이터를 Float32Array로 변환
 *
 * ArrayBuffer 형태의 WAV 데이터를 Float32Array로 변환합니다.
 * 16-bit PCM 형식을 가정합니다.
 *
 * @param {ArrayBuffer} wavData - WAV 형식 오디오 데이터
 * @returns {Float32Array} 변환된 오디오 샘플 (-1.0 ~ 1.0)
 */
export function wavToFloat32(wavData) {
    // WAV 헤더 스킵 (44 bytes)
    const dataView = new DataView(wavData);
    const samples = new Int16Array(wavData, 44);

    // Int16 (-32768 ~ 32767)을 Float32 (-1.0 ~ 1.0)로 변환
    const float32 = new Float32Array(samples.length);
    for (let i = 0; i < samples.length; i++) {
        float32[i] = samples[i] / 32768.0;
    }

    return float32;
}

/**
 * Float32Array를 Int16Array로 변환 (WAV 저장용)
 *
 * @param {Float32Array} float32Data - Float32 오디오 샘플
 * @returns {Int16Array} 변환된 Int16 샘플
 */
export function float32ToInt16(float32Data) {
    const int16 = new Int16Array(float32Data.length);
    for (let i = 0; i < float32Data.length; i++) {
        // Clamp to [-1.0, 1.0]
        const clamped = Math.max(-1.0, Math.min(1.0, float32Data[i]));
        int16[i] = Math.round(clamped * 32767);
    }
    return int16;
}

/**
 * 오디오 정규화 (RMS 기반)
 *
 * @param {Float32Array} audioData - 입력 오디오 데이터
 * @param {number} targetRMS - 목표 RMS 값 (기본 0.1)
 * @returns {Float32Array} 정규화된 오디오 데이터
 */
export function normalizeAudioByRMS(audioData, targetRMS = 0.1) {
    // RMS 계산
    let sum = 0;
    for (let i = 0; i < audioData.length; i++) {
        sum += audioData[i] * audioData[i];
    }
    const rms = Math.sqrt(sum / audioData.length);

    if (rms < 0.0001) {
        // 거의 무음인 경우 그대로 반환
        return audioData;
    }

    // 스케일 계산 및 적용
    const scale = targetRMS / rms;
    const normalized = new Float32Array(audioData.length);
    for (let i = 0; i < audioData.length; i++) {
        normalized[i] = audioData[i] * scale;
    }

    return normalized;
}

/**
 * 오디오 정규화 (Peak 기반)
 *
 * @param {Float32Array} audioData - 입력 오디오 데이터
 * @param {number} targetPeak - 목표 Peak 값 (기본 0.95)
 * @returns {Float32Array} 정규화된 오디오 데이터
 */
export function normalizeAudioByPeak(audioData, targetPeak = 0.95) {
    // Peak 찾기
    let peak = 0;
    for (let i = 0; i < audioData.length; i++) {
        const abs = Math.abs(audioData[i]);
        if (abs > peak) {
            peak = abs;
        }
    }

    if (peak < 0.0001) {
        // 거의 무음인 경우 그대로 반환
        return audioData;
    }

    // 스케일 계산 및 적용
    const scale = targetPeak / peak;
    const normalized = new Float32Array(audioData.length);
    for (let i = 0; i < audioData.length; i++) {
        normalized[i] = audioData[i] * scale;
    }

    return normalized;
}

/**
 * 오디오 믹싱 (두 개의 오디오를 합침)
 *
 * @param {Float32Array} audio1 - 첫 번째 오디오
 * @param {Float32Array} audio2 - 두 번째 오디오
 * @param {number} ratio1 - 첫 번째 오디오 비율 (0.0 ~ 1.0)
 * @param {number} ratio2 - 두 번째 오디오 비율 (0.0 ~ 1.0)
 * @returns {Float32Array} 믹싱된 오디오
 */
export function mixAudio(audio1, audio2, ratio1 = 0.5, ratio2 = 0.5) {
    const maxLength = Math.max(audio1.length, audio2.length);
    const mixed = new Float32Array(maxLength);

    for (let i = 0; i < maxLength; i++) {
        const sample1 = i < audio1.length ? audio1[i] : 0;
        const sample2 = i < audio2.length ? audio2[i] : 0;
        mixed[i] = sample1 * ratio1 + sample2 * ratio2;
    }

    return mixed;
}
