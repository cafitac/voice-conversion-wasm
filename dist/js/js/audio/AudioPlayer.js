import { AudioBuffer } from './AudioBuffer.js';

/**
 * AudioPlayer - 오디오 재생 (Web Audio API 사용)
 */
export class AudioPlayer {
    constructor() {
        this.audioContext = null;
        this.sourceNode = null;
        this.isPlaying = false;
    }

    /**
     * AudioContext 초기화
     */
    initContext() {
        if (!this.audioContext) {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        }
    }

    /**
     * 오디오 재생
     * @param {AudioBuffer} audioBuffer
     */
    play(audioBuffer) {
        this.stop();
        this.initContext();

        // Create buffer source
        this.sourceNode = this.audioContext.createBufferSource();

        // Create Web Audio API AudioBuffer
        const buffer = this.audioContext.createBuffer(
            audioBuffer.getChannels(),
            audioBuffer.getLength(),
            audioBuffer.getSampleRate()
        );

        // Copy data
        const channelData = buffer.getChannelData(0);
        const data = audioBuffer.getData();
        for (let i = 0; i < data.length; i++) {
            channelData[i] = data[i];
        }

        this.sourceNode.buffer = buffer;
        this.sourceNode.connect(this.audioContext.destination);

        this.sourceNode.onended = () => {
            this.isPlaying = false;
        };

        this.sourceNode.start(0);
        this.isPlaying = true;
    }

    /**
     * 재생 중지
     */
    stop() {
        if (this.sourceNode && this.isPlaying) {
            try {
                this.sourceNode.stop();
            } catch (e) {
                // Already stopped
            }
            this.isPlaying = false;
        }
    }

    /**
     * 재생 중인지 확인
     * @returns {boolean}
     */
    getIsPlaying() {
        return this.isPlaying;
    }
}
