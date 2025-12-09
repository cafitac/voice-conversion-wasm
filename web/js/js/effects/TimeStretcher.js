import { AudioBuffer } from '../audio/AudioBuffer.js';

/**
 * TimeStretcher - 시간 늘이기/줄이기 (구현 예정)
 */
export class TimeStretcher {
    constructor() {}

    /**
     * Time stretch 적용 (준비 중)
     * @param {AudioBuffer} input
     * @param {number} ratio - 비율 (0.5 ~ 2.0, 1.0 = 변화 없음)
     * @returns {AudioBuffer}
     */
    apply(input, ratio) {
        console.warn('TimeStretcher.apply() - 준비 중입니다.');
        // TODO: 직접 구현 예정
        return input.clone();
    }
}
