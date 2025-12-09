import { AudioBuffer } from '../audio/AudioBuffer.js';

/**
 * PitchShifter - 피치 변경 (구현 예정)
 */
export class PitchShifter {
    constructor() {}

    /**
     * Pitch shift 적용 (준비 중)
     * @param {AudioBuffer} input
     * @param {number} semitones - 반음 단위 (-12 ~ +12)
     * @returns {AudioBuffer}
     */
    apply(input, semitones) {
        console.warn('PitchShifter.apply() - 준비 중입니다.');
        // TODO: 직접 구현 예정
        return input.clone();
    }
}
