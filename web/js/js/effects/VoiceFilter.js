import { AudioBuffer } from '../audio/AudioBuffer.js';

/**
 * FilterType enum
 */
export const FilterType = {
    LOW_PASS: 0,
    HIGH_PASS: 1,
    BAND_PASS: 2,
    ROBOT: 3,
    ECHO: 4,
    REVERB: 5,
    DISTORTION: 6,
    AM_RADIO: 7,
    CHORUS: 8,
    FLANGER: 9,
    VOICE_CHANGER_MALE_TO_FEMALE: 10,
    VOICE_CHANGER_FEMALE_TO_MALE: 11
};

/**
 * VoiceFilter - 음성 필터 효과 (JavaScript 버전)
 */
export class VoiceFilter {
    constructor() {}

    /**
     * 필터 적용
     * @param {AudioBuffer} input
     * @param {number} type - FilterType
     * @param {number} param1
     * @param {number} param2
     * @returns {AudioBuffer}
     */
    applyFilter(input, type, param1 = 0.5, param2 = 0.5) {
        switch (type) {
            case FilterType.LOW_PASS:
                return this.applyLowPass(input, param1);
            case FilterType.HIGH_PASS:
                return this.applyHighPass(input, param1);
            case FilterType.BAND_PASS:
                return this.applyBandPass(input, param1, param2);
            case FilterType.ROBOT:
                return this.applyRobot(input);
            case FilterType.ECHO:
                return this.applyEcho(input, param1, param2);
            case FilterType.REVERB:
                return this.applyReverb(input, param1, param2);
            case FilterType.DISTORTION:
                return this.applyDistortion(input, param1, param2);
            case FilterType.AM_RADIO:
                return this.applyAMRadio(input, param1, param2);
            case FilterType.CHORUS:
                return this.applyChorus(input, param1, param2);
            case FilterType.FLANGER:
                return this.applyFlanger(input, param1, param2);
            case FilterType.VOICE_CHANGER_MALE_TO_FEMALE:
                return this.applyVoiceChangerMaleToFemale(input, param1);
            case FilterType.VOICE_CHANGER_FEMALE_TO_MALE:
                return this.applyVoiceChangerFemaleToMale(input, param1);
            default:
                return input.clone();
        }
    }

    /**
     * Low-pass filter
     */
    applyLowPass(input, cutoff) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        // Simple first-order low-pass filter
        const rc = 1.0 / (cutoff * 2 * Math.PI * 5000);
        const dt = 1.0 / sampleRate;
        const alpha = dt / (rc + dt);

        for (let i = 1; i < data.length; i++) {
            data[i] = data[i - 1] + alpha * (data[i] - data[i - 1]);
        }

        return output;
    }

    /**
     * High-pass filter
     */
    applyHighPass(input, cutoff) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        // Simple first-order high-pass filter
        const rc = 1.0 / (cutoff * 2 * Math.PI * 5000);
        const dt = 1.0 / sampleRate;
        const alpha = rc / (rc + dt);

        let prevInput = 0;
        let prevOutput = 0;

        for (let i = 0; i < data.length; i++) {
            const currentInput = data[i];
            data[i] = alpha * (prevOutput + currentInput - prevInput);
            prevInput = currentInput;
            prevOutput = data[i];
        }

        return output;
    }

    /**
     * Band-pass filter
     */
    applyBandPass(input, lowCutoff, highCutoff) {
        // Low cutoff 적용 후 high cutoff 적용
        let temp = this.applyHighPass(input, lowCutoff);
        return this.applyLowPass(temp, highCutoff);
    }

    /**
     * Robot voice effect
     */
    applyRobot(input) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        // Ring modulation with carrier frequency
        const carrierFreq = 30; // Hz
        for (let i = 0; i < data.length; i++) {
            const t = i / sampleRate;
            const carrier = Math.sin(2 * Math.PI * carrierFreq * t);
            data[i] *= carrier;
        }

        return output;
    }

    /**
     * Echo effect
     */
    applyEcho(input, delay, feedback) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        const delaySamples = Math.floor(delay * sampleRate * 0.5); // 0~0.5초

        for (let i = delaySamples; i < data.length; i++) {
            data[i] += data[i - delaySamples] * feedback * 0.6;
        }

        return output;
    }

    /**
     * Reverb effect
     */
    applyReverb(input, roomSize, damping) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        // Simple comb filter delays
        const delays = [
            Math.floor(sampleRate * 0.0297 * (0.5 + roomSize * 0.5)),
            Math.floor(sampleRate * 0.0371 * (0.5 + roomSize * 0.5)),
            Math.floor(sampleRate * 0.0411 * (0.5 + roomSize * 0.5)),
            Math.floor(sampleRate * 0.0437 * (0.5 + roomSize * 0.5))
        ];

        const feedback = 0.5 - damping * 0.3;

        delays.forEach(delaySamples => {
            for (let i = delaySamples; i < data.length; i++) {
                data[i] += data[i - delaySamples] * feedback * 0.25;
            }
        });

        return output;
    }

    /**
     * Distortion effect
     */
    applyDistortion(input, drive, tone) {
        const output = input.clone();
        const data = output.getData();

        const amount = 1 + drive * 20;

        for (let i = 0; i < data.length; i++) {
            let sample = data[i] * amount;
            // Soft clipping
            sample = Math.tanh(sample);
            data[i] = sample * 0.7;
        }

        // Tone control (simple low-pass)
        if (tone < 0.5) {
            return this.applyLowPass(output, tone);
        }

        return output;
    }

    /**
     * AM Radio effect
     */
    applyAMRadio(input, noiseLevel, bandwidth) {
        // Band-pass filter + noise
        let output = this.applyBandPass(input, 0.1, 0.3 + bandwidth * 0.3);
        const data = output.getData();

        // Add noise
        for (let i = 0; i < data.length; i++) {
            data[i] += (Math.random() * 2 - 1) * noiseLevel * 0.1;
        }

        return output;
    }

    /**
     * Chorus effect
     */
    applyChorus(input, rate, depth) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        const lfoFreq = rate * 2 + 0.5; // 0.5 ~ 2.5 Hz
        const maxDelay = Math.floor(sampleRate * 0.03 * depth); // Up to 30ms

        for (let i = maxDelay; i < data.length; i++) {
            const t = i / sampleRate;
            const lfo = Math.sin(2 * Math.PI * lfoFreq * t);
            const delaySamples = Math.floor(maxDelay * (0.5 + 0.5 * lfo));

            if (i - delaySamples >= 0) {
                data[i] = 0.7 * data[i] + 0.3 * data[i - delaySamples];
            }
        }

        return output;
    }

    /**
     * Flanger effect
     */
    applyFlanger(input, rate, depth) {
        const output = input.clone();
        const data = output.getData();
        const sampleRate = input.getSampleRate();

        const lfoFreq = rate * 0.5 + 0.1; // 0.1 ~ 0.6 Hz
        const maxDelay = Math.floor(sampleRate * 0.01 * depth); // Up to 10ms

        for (let i = maxDelay; i < data.length; i++) {
            const t = i / sampleRate;
            const lfo = Math.sin(2 * Math.PI * lfoFreq * t);
            const delaySamples = Math.floor(maxDelay * (0.5 + 0.5 * lfo));

            if (i - delaySamples >= 0) {
                data[i] = 0.5 * data[i] + 0.5 * data[i - delaySamples];
            }
        }

        return output;
    }

    /**
     * Voice changer: Male to Female
     */
    applyVoiceChangerMaleToFemale(input, intensity) {
        // 뉴스 인터뷰 스타일: Band-pass + slight echo
        let output = this.applyBandPass(input, 0.15, 0.55);
        output = this.applyEcho(output, 0.3 * intensity, 0.2);

        // Normalize
        this.normalize(output);

        return output;
    }

    /**
     * Voice changer: Female to Male
     */
    applyVoiceChangerFemaleToMale(input, intensity) {
        // 범인 목소리: Low-pass + distortion
        let output = this.applyLowPass(input, 0.3);
        output = this.applyDistortion(output, 0.3 * intensity, 0.3);

        // Normalize
        this.normalize(output);

        return output;
    }

    /**
     * Normalize audio to prevent clipping
     */
    normalize(audioBuffer) {
        const data = audioBuffer.getData();
        let max = 0;

        for (let i = 0; i < data.length; i++) {
            max = Math.max(max, Math.abs(data[i]));
        }

        if (max > 1.0) {
            for (let i = 0; i < data.length; i++) {
                data[i] /= max;
            }
        }
    }

    /**
     * Calculate RMS (Root Mean Square)
     */
    calculateRMS(data) {
        let sum = 0;
        for (let i = 0; i < data.length; i++) {
            sum += data[i] * data[i];
        }
        return Math.sqrt(sum / data.length);
    }
}
