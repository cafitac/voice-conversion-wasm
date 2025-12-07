import { AudioBuffer } from './AudioBuffer.js';

/**
 * AudioRecorder - 오디오 녹음 (Web Audio API 사용)
 */
export class AudioRecorder {
    constructor() {
        this.mediaRecorder = null;
        this.audioChunks = [];
        this.stream = null;
        this.isRecording = false;
    }

    /**
     * 녹음 시작
     */
    async start() {
        try {
            this.stream = await navigator.mediaDevices.getUserMedia({ audio: true });
            this.mediaRecorder = new MediaRecorder(this.stream);
            this.audioChunks = [];

            this.mediaRecorder.ondataavailable = (event) => {
                this.audioChunks.push(event.data);
            };

            this.mediaRecorder.start();
            this.isRecording = true;

            console.log('Recording started');
        } catch (error) {
            console.error('Error starting recording:', error);
            throw error;
        }
    }

    /**
     * 녹음 중지 및 AudioBuffer 반환
     * @returns {Promise<AudioBuffer>}
     */
    async stop() {
        return new Promise((resolve, reject) => {
            if (!this.mediaRecorder || !this.isRecording) {
                reject(new Error('Recording not started'));
                return;
            }

            this.mediaRecorder.onstop = async () => {
                try {
                    const audioBlob = new Blob(this.audioChunks, { type: 'audio/webm' });
                    const arrayBuffer = await audioBlob.arrayBuffer();

                    // Decode audio data
                    const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                    const audioBufferData = await audioContext.decodeAudioData(arrayBuffer);

                    // Convert to mono Float32Array
                    const sampleRate = audioBufferData.sampleRate;
                    const channelData = audioBufferData.getChannelData(0);

                    const buffer = new AudioBuffer(sampleRate, 1);
                    buffer.setData(channelData);

                    this.isRecording = false;

                    // Stop all tracks
                    if (this.stream) {
                        this.stream.getTracks().forEach(track => track.stop());
                    }

                    console.log('Recording stopped');
                    resolve(buffer);
                } catch (error) {
                    console.error('Error processing recording:', error);
                    reject(error);
                }
            };

            this.mediaRecorder.stop();
        });
    }

    /**
     * 녹음 중인지 확인
     * @returns {boolean}
     */
    getIsRecording() {
        return this.isRecording;
    }
}
