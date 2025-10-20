export class AudioPlayer {
    constructor() {
        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        this.currentSource = null;
    }

    async playBuffer(audioBuffer) {
        // 재생 중이면 중지
        this.stop();

        const source = this.audioContext.createBufferSource();
        source.buffer = audioBuffer;
        source.connect(this.audioContext.destination);
        source.start(0);

        this.currentSource = source;

        return new Promise((resolve) => {
            source.onended = () => {
                this.currentSource = null;
                resolve();
            };
        });
    }

    async playWavData(wavData) {
        const audioBuffer = await this.decodeWavData(wavData);
        return this.playBuffer(audioBuffer);
    }

    async playFloat32Array(float32Data, sampleRate = 44100) {
        const audioBuffer = this.audioContext.createBuffer(1, float32Data.length, sampleRate);
        audioBuffer.getChannelData(0).set(float32Data);
        return this.playBuffer(audioBuffer);
    }

    async decodeWavData(wavData) {
        // Uint8Array를 ArrayBuffer로 변환
        const arrayBuffer = wavData.buffer.slice(
            wavData.byteOffset,
            wavData.byteOffset + wavData.byteLength
        );
        return await this.audioContext.decodeAudioData(arrayBuffer);
    }

    stop() {
        if (this.currentSource) {
            try {
                this.currentSource.stop();
            } catch (e) {
                // 이미 중지된 경우 무시
            }
            this.currentSource = null;
        }
    }

    downloadWav(wavData, filename = 'audio.wav') {
        const blob = new Blob([wavData], { type: 'audio/wav' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }
}
