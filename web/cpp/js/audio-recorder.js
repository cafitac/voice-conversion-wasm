export class AudioRecorder {
    constructor(module) {
        this.module = module;
        this.audioContext = null;
        this.mediaStream = null;
        this.processor = null;
        this.isRecording = false;
        this.recordedBuffer = null;
        this.chunks = [];
        this.sampleRate = 48000;
    }

    async init() {
        try {
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            this.mediaStream = await navigator.mediaDevices.getUserMedia({ audio: true });
            return true;
        } catch (error) {
            console.error('마이크 접근 실패:', error);
            // 실패 시 명시적으로 null로 설정
            this.audioContext = null;
            this.mediaStream = null;
            return false;
        }
    }

    async startRecording() {
        if (!this.audioContext || !this.mediaStream) {
            const success = await this.init();
            if (!success) {
                throw new Error('마이크 접근 권한이 필요합니다. 브라우저에서 마이크 권한을 허용해주세요.');
            }
        }

        // AudioContext가 suspended 상태면 resume
        if (this.audioContext.state === 'suspended') {
            await this.audioContext.resume();
        }

        // MediaStream 유효성 재확인
        if (!this.mediaStream) {
            throw new Error('MediaStream이 초기화되지 않았습니다.');
        }

        this.isRecording = true;
        this.chunks = [];
        this.sampleRate = this.audioContext.sampleRate || 48000;

        const source = this.audioContext.createMediaStreamSource(this.mediaStream);

        // ScriptProcessor 사용 (더 나은 호환성)
        const bufferSize = 4096;
        this.processor = this.audioContext.createScriptProcessor(bufferSize, 1, 1);

        this.processor.onaudioprocess = (event) => {
            if (this.isRecording) {
                const inputData = event.inputBuffer.getChannelData(0);
                // 입력 데이터를 복사해서 로컬 버퍼에 쌓아 둠
                this.chunks.push(new Float32Array(inputData));
            }
        };

        source.connect(this.processor);
        this.processor.connect(this.audioContext.destination);
    }

    stopRecording() {
        this.isRecording = false;

        if (this.processor) {
            this.processor.disconnect();
            this.processor = null;
        }

        // JS 측에서 쌓아 둔 Float32Array 조각들을 하나로 병합
        if (this.chunks.length > 0) {
            const totalLength = this.chunks.reduce((sum, chunk) => sum + chunk.length, 0);
            const result = new Float32Array(totalLength);
            let offset = 0;
            for (const chunk of this.chunks) {
                result.set(chunk, offset);
                offset += chunk.length;
            }
            this.recordedBuffer = result;
        } else {
            this.recordedBuffer = null;
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
