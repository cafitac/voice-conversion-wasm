/**
 * AudioBuffer - 오디오 데이터 관리 클래스 (JavaScript 버전)
 */
export class AudioBuffer {
    constructor(sampleRate = 48000, channels = 1) {
        this.sampleRate = sampleRate;
        this.channels = channels;
        this.data = new Float32Array(0);
    }

    /**
     * 오디오 데이터 설정
     * @param {Float32Array|Array} samples
     */
    setData(samples) {
        if (samples instanceof Float32Array) {
            this.data = samples;
        } else {
            this.data = new Float32Array(samples);
        }
    }

    /**
     * 오디오 데이터 가져오기
     * @returns {Float32Array}
     */
    getData() {
        return this.data;
    }

    /**
     * 샘플 수
     * @returns {number}
     */
    getLength() {
        return this.data.length;
    }

    /**
     * 길이 (초)
     * @returns {number}
     */
    getDuration() {
        return this.data.length / this.sampleRate;
    }

    /**
     * 샘플레이트
     * @returns {number}
     */
    getSampleRate() {
        return this.sampleRate;
    }

    /**
     * 채널 수
     * @returns {number}
     */
    getChannels() {
        return this.channels;
    }

    /**
     * 복사본 생성
     * @returns {AudioBuffer}
     */
    clone() {
        const cloned = new AudioBuffer(this.sampleRate, this.channels);
        cloned.setData(new Float32Array(this.data));
        return cloned;
    }
}
