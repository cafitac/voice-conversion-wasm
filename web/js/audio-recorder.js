export class AudioRecorder {
    constructor(module) {
        this.module = module;
        this.audioContext = null;
        this.mediaStream = null;
        this.processor = null;
        this.isRecording = false;
        this.recordedBuffer = null;
    }

    async init() {
        try {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            this.mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true });
            return true;
        } catch (error) {
            console.error('마이크 접근 실패:', error);
            return false;
        }
    }

    async startRecording() {
        if (!this.audioContext) {
            await this.init();
        }

        // AudioContext가 suspended 상태면 resume
        if (this.audioContext.state === 'suspended') {
            await this.audioContext.resume();
        }

        this.isRecording = true;
        this.module.startRecording();

        const source = this.audioContext.createMediaStreamSource(this.mediaStream);

        // ScriptProcessor 사용 (더 나은 호환성)
        const bufferSize = 4096;
        this.processor = this.audioContext.createScriptProcessor(bufferSize, 1, 1);

        this.processor.onaudioprocess = (event) => {
            if (this.isRecording) {
                const inputData = event.inputBuffer.getChannelData(0);
                this.sendToWasm(inputData);
            }
        };

        source.connect(this.processor);
        this.processor.connect(this.audioContext.destination);
    }

    sendToWasm(audioData) {
        // Float32Array를 WASM 메모리에 복사
        const dataPtr = this.module._malloc(audioData.length * 4);
        this.module.HEAPF32.set(audioData, dataPtr / 4);

        this.module.addAudioData(dataPtr, audioData.length);
        this.module._free(dataPtr);
    }

    stopRecording() {
        this.isRecording = false;
        this.module.stopRecording();

        if (this.processor) {
            this.processor.disconnect();
            this.processor = null;
        }

        // 녹음된 데이터 가져오기
        const wavData = this.module.getRecordedAudioAsWav();

        if (wavData) {
            this.recordedBuffer = new Uint8Array(wavData);
        }

        return this.recordedBuffer;
    }

    getRecordedBuffer() {
        return this.recordedBuffer;
    }

    destroy() {
        if (this.mediaStream) {
            this.mediaStream.getTracks().forEach(track => track.stop());
        }
        if (this.audioContext) {
            this.audioContext.close();
        }
    }
}
