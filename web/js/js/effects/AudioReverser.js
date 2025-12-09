import { AudioBuffer } from '../audio/AudioBuffer.js';

/**
 * AudioReverser - 오디오 역재생 (JavaScript 버전)
 */
export class AudioReverser {
    constructor() {}

    /**
     * 오디오를 역재생
     * @param {AudioBuffer} input
     * @returns {AudioBuffer}
     */
    reverse(input) {
        const output = new AudioBuffer(input.getSampleRate(), input.getChannels());
        const data = input.getData();
        const reversed = new Float32Array(data.length);

        // 샘플을 역순으로 배치
        for (let i = 0; i < data.length; i++) {
            reversed[i] = data[data.length - 1 - i];
        }

        output.setData(reversed);
        return output;
    }
}
