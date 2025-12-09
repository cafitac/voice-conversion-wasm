import { AudioBuffer } from '../audio/AudioBuffer.js';

/**
 * PitchAnalyzer - 피치 분석
 * C++ 버전을 JavaScript로 완전히 동일하게 포팅
 *
 * Autocorrelation 기반 피치 검출 알고리즘:
 * 1. 오디오를 작은 프레임으로 나눔
 * 2. 각 프레임에서 자기상관(autocorrelation) 계산
 * 3. 주기성을 찾아서 주파수 추정
 * 4. Median filter로 노이즈 제거
 */
export class PitchAnalyzer {
    constructor() {
        this.minFreq = 80.0;   // 최소 주파수 (Hz)
        this.maxFreq = 400.0;  // 최대 주파수 (Hz)
    }

    /**
     * 오디오 버퍼 분석
     * @param {AudioBuffer} buffer - 입력 오디오
     * @param {number} frameSize - 프레임 크기 (초 단위, 기본값 0.02 = 20ms)
     * @returns {Array<{time: number, frequency: number, confidence: number}>}
     */
    analyze(buffer, frameSize = 0.02) {
        const pitchPoints = [];

        const data = buffer.getData();
        const sampleRate = buffer.getSampleRate();
        const frameLength = Math.floor(frameSize * sampleRate);
        const hopSize = Math.floor(frameLength / 2); // 50% overlap

        console.log('[PitchAnalyzer] 분석 시작 - 샘플레이트:', sampleRate, 'Hz, 프레임 길이:', frameLength, '샘플');

        for (let i = 0; i + frameLength < data.length; i += hopSize) {
            const frame = data.slice(i, i + frameLength);

            const result = this.extractPitch(frame, sampleRate, this.minFreq, this.maxFreq);

            if (result.frequency > 0.0) {
                const point = {
                    time: i / sampleRate,
                    frequency: result.frequency,
                    confidence: result.confidence
                };
                pitchPoints.push(point);
            }
        }

        console.log('[PitchAnalyzer] 분석 완료 - 검출된 피치 포인트:', pitchPoints.length, '개');

        // Median filter 적용하여 튀는 값 제거
        return this.applyMedianFilter(pitchPoints, 5);
    }

    /**
     * 단일 프레임에서 피치 추출
     * @param {Float32Array} frame - 오디오 프레임
     * @param {number} sampleRate - 샘플레이트
     * @param {number} minFreq - 최소 주파수
     * @param {number} maxFreq - 최대 주파수
     * @returns {{frequency: number, confidence: number}}
     */
    extractPitch(frame, sampleRate, minFreq, maxFreq) {
        const result = {
            frequency: 0.0,
            confidence: 0.0
        };

        if (frame.length === 0) return result;

        // Autocorrelation 계산
        const autocorr = this.calculateAutocorrelation(frame);

        // 탐색 범위 계산
        const minLag = Math.floor(sampleRate / maxFreq);
        let maxLag = Math.floor(sampleRate / minFreq);

        if (maxLag >= autocorr.length) {
            maxLag = autocorr.length - 1;
        }

        // 최대 피크 찾기
        let peakLag = minLag;
        let maxValue = autocorr[minLag];

        for (let lag = minLag; lag <= maxLag; lag++) {
            if (autocorr[lag] > maxValue) {
                maxValue = autocorr[lag];
                peakLag = lag;
            }
        }

        // Confidence 계산: autocorrelation 피크 값 사용
        // autocorr은 이미 정규화되어 있으므로 0~1 범위
        result.confidence = maxValue;

        // Parabolic interpolation으로 정확한 피크 위치 찾기
        const refinedLag = this.findPeakParabolic(autocorr, peakLag);

        // 주파수 계산
        if (refinedLag > 0) {
            result.frequency = sampleRate / refinedLag;
        }

        return result;
    }

    /**
     * 최소/최대 주파수 설정
     */
    setMinFrequency(freq) {
        this.minFreq = freq;
    }

    setMaxFrequency(freq) {
        this.maxFreq = freq;
    }

    /**
     * Autocorrelation 계산
     * 신호의 자기상관을 계산하여 주기성을 찾음
     *
     * 원리:
     * - 신호를 자기 자신과 여러 lag에서 곱함
     * - 주기적인 신호는 특정 lag에서 높은 상관관계를 보임
     * - 이 lag 값이 주기(period)가 됨
     *
     * @param {Float32Array} signal - 입력 신호
     * @returns {Float32Array} 자기상관 결과
     */
    calculateAutocorrelation(signal) {
        const n = signal.length;
        const autocorr = new Float32Array(n);

        for (let lag = 0; lag < n; lag++) {
            let sum = 0.0;
            for (let i = 0; i < n - lag; i++) {
                sum += signal[i] * signal[i + lag];
            }
            autocorr[lag] = sum;
        }

        // 정규화
        if (autocorr[0] > 0.0) {
            const norm = autocorr[0];
            for (let i = 0; i < n; i++) {
                autocorr[i] /= norm;
            }
        }

        return autocorr;
    }

    /**
     * Parabolic interpolation으로 피크의 정확한 위치 찾기
     *
     * 이산(discrete) 샘플 사이의 실제 피크 위치를 추정
     * 3개의 점으로 포물선을 그려서 정점 찾기
     *
     * @param {Float32Array} data - 데이터 배열
     * @param {number} index - 피크 인덱스
     * @returns {number} 보정된 피크 위치
     */
    findPeakParabolic(data, index) {
        if (index <= 0 || index >= data.length - 1) {
            return index;
        }

        const alpha = data[index - 1];
        const beta = data[index];
        const gamma = data[index + 1];

        // Parabolic interpolation
        // 3점을 지나는 포물선의 정점 계산
        const offset = 0.5 * (alpha - gamma) / (alpha - 2.0 * beta + gamma);

        return index + offset;
    }

    /**
     * Median filter 적용
     *
     * 튀는 값(outlier)을 제거하여 부드러운 피치 곡선 생성
     * 각 점을 주변 windowSize개 점들의 중간값으로 대체
     *
     * @param {Array} points - 피치 포인트 배열
     * @param {number} windowSize - 윈도우 크기 (홀수 권장)
     * @returns {Array} 필터링된 포인트 배열
     */
    applyMedianFilter(points, windowSize) {
        if (points.length < windowSize) {
            return points;
        }

        const filtered = [];
        const halfWindow = Math.floor(windowSize / 2);

        for (let i = 0; i < points.length; i++) {
            // 윈도우 범위 계산
            const start = Math.max(0, i - halfWindow);
            const end = Math.min(points.length - 1, i + halfWindow);

            // 윈도우 내 frequency 값들 수집
            const windowFreqs = [];
            for (let j = start; j <= end; j++) {
                windowFreqs.push(points[j].frequency);
            }

            // Median 계산
            windowFreqs.sort((a, b) => a - b);
            const median = windowFreqs[Math.floor(windowFreqs.length / 2)];

            // 필터링된 포인트 생성
            const filteredPoint = {
                time: points[i].time,
                frequency: median,
                confidence: points[i].confidence
            };
            filtered.push(filteredPoint);
        }

        return filtered;
    }
}
