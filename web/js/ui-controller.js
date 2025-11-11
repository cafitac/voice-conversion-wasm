import { AudioRecorder } from './audio-recorder.js';
import { AudioPlayer } from './audio-player.js';

export class UIController {
    constructor() {
        this.recorder = null;
        this.player = new AudioPlayer();
        this.module = null;
        this.originalAudio = null;
        this.processedAudio = null;
        this.currentAudioData = null;
        this.sampleRate = 48000; // 브라우저 기본값
        this.audioMaxTime = 0;
        this.cachedAudioPtr = null;
        this.cachedAudioLength = 0;
    }

    async init() {
        // WASM 모듈 로드 대기
        if (typeof Module === 'undefined') {
            console.error('Module is not defined! main.js may not have loaded properly.');
            return;
        }

        this.module = Module;
        this.recorder = new AudioRecorder(this.module);
        this.module.init();
        this.setupEventListeners();

        // 초기 Pitch 품질 설정 (기본값: external)
        this.setPitchQuality('external');
    }

    setupEventListeners() {
        console.log('Setting up event listeners...');

        // 녹음 버튼
        document.getElementById('startRecord').addEventListener('click', () => this.startRecording());
        document.getElementById('stopRecord').addEventListener('click', () => this.stopRecording());
        document.getElementById('uploadFile').addEventListener('click', () => this.uploadFile());
        document.getElementById('fileInput').addEventListener('change', (e) => this.handleFileUpload(e));
        document.getElementById('playOriginal').addEventListener('click', () => this.playOriginal());
        document.getElementById('downloadOriginal').addEventListener('click', () => this.downloadOriginal());

        // 분석 버튼
        document.getElementById('analyzeVoice').addEventListener('click', () => this.analyzeVoice());

        // 효과 버튼
        document.getElementById('applyPitchShift').addEventListener('click', () => this.applyPitchShift());
        document.getElementById('applyTimeStretch').addEventListener('click', () => this.applyTimeStretch());
        document.getElementById('applyFilter').addEventListener('click', () => this.applyFilter());

        // 재생 및 다운로드
        document.getElementById('playProcessed').addEventListener('click', () => this.playProcessed());
        document.getElementById('downloadProcessed').addEventListener('click', () => this.downloadProcessed());
        document.getElementById('reset').addEventListener('click', () => this.reset());

        // 슬라이더 값 업데이트
        document.getElementById('pitchShift').addEventListener('input', (e) => {
            document.getElementById('pitchValue').textContent = e.target.value;
        });
        document.getElementById('timeStretch').addEventListener('input', (e) => {
            document.getElementById('timeValue').textContent = e.target.value;
        });

        // Pitch 품질 선택
        document.getElementById('pitchQuality').addEventListener('change', (e) => {
            this.setPitchQuality(e.target.value);
        });
        document.getElementById('filterParam1').addEventListener('input', (e) => {
            document.getElementById('param1Value').textContent = e.target.value;
        });
        document.getElementById('filterParam2').addEventListener('input', (e) => {
            document.getElementById('param2Value').textContent = e.target.value;
        });
    }

    async startRecording() {
        document.getElementById('recordStatus').textContent = '마이크 권한 요청 중...';
        document.getElementById('startRecord').disabled = true;
        document.getElementById('stopRecord').disabled = false;

        try {
            await this.recorder.startRecording();
            document.getElementById('recordStatus').textContent = '녹음 중...';
        } catch (error) {
            console.error('녹음 시작 실패:', error);

            // 버튼 상태 복원
            document.getElementById('startRecord').disabled = false;
            document.getElementById('stopRecord').disabled = true;

            // 에러 메시지 표시
            const errorMsg = '녹음 시작 실패: ' + error.message;
            document.getElementById('recordStatus').textContent = errorMsg;
            document.getElementById('recordStatus').style.color = '#f44336';

            // Alert도 표시
            alert('🎤 마이크 접근 권한이 필요합니다.\n\n' +
                  '브라우저 설정에서 마이크 권한을 허용해주세요.\n\n' +
                  '1. 주소창 왼쪽의 자물쇠/정보 아이콘을 클릭\n' +
                  '2. 마이크 권한을 "허용"으로 변경\n' +
                  '3. 페이지를 새로고침');

            // 3초 후 상태 메시지 색상 복원
            setTimeout(() => {
                document.getElementById('recordStatus').style.color = '';
            }, 3000);
        }
    }

    stopRecording() {
        this.originalAudio = this.recorder.stopRecording();
        this.currentAudioData = this.originalAudio;

        document.getElementById('recordStatus').textContent = '녹음 완료!';
        document.getElementById('startRecord').disabled = false;
        document.getElementById('stopRecord').disabled = true;
        document.getElementById('playOriginal').disabled = false;
        document.getElementById('downloadOriginal').disabled = false;
        document.getElementById('analyzeVoice').disabled = false;
        document.getElementById('applyPitchShift').disabled = false;
        document.getElementById('applyTimeStretch').disabled = false;
        document.getElementById('applyFilter').disabled = false;

        // 파형 그리기
        this.drawWaveform(this.originalAudio);
    }

    uploadFile() {
        document.getElementById('fileInput').click();
    }

    async handleFileUpload(event) {
        const file = event.target.files[0];
        if (!file) return;

        document.getElementById('recordStatus').textContent = '파일 로딩 중...';

        try {
            const arrayBuffer = await file.arrayBuffer();
            const wavData = new Uint8Array(arrayBuffer);

            // WAV 헤더 검증 (RIFF, WAVE)
            const view = new DataView(wavData.buffer);
            const riff = String.fromCharCode(...wavData.slice(0, 4));
            const wave = String.fromCharCode(...wavData.slice(8, 12));

            if (riff !== 'RIFF' || wave !== 'WAVE') {
                throw new Error('유효한 WAV 파일이 아닙니다.');
            }

            // Sample Rate 추출 (offset 24, 4 bytes, little-endian)
            this.sampleRate = view.getUint32(24, true);

            this.originalAudio = wavData;
            this.currentAudioData = wavData;

            document.getElementById('recordStatus').textContent = `파일 업로드 완료! (${file.name}, ${this.sampleRate}Hz)`;
            document.getElementById('playOriginal').disabled = false;
            document.getElementById('downloadOriginal').disabled = false;
            document.getElementById('analyzeVoice').disabled = false;
            document.getElementById('applyPitchShift').disabled = false;
            document.getElementById('applyTimeStretch').disabled = false;
            document.getElementById('applyFilter').disabled = false;

            // 파형 그리기
            this.drawWaveform(this.originalAudio);
        } catch (error) {
            console.error('파일 업로드 실패:', error);
            document.getElementById('recordStatus').textContent = '파일 업로드 실패: ' + error.message;
        }
    }

    drawWaveform(wavData) {
        // WAV 데이터를 Canvas에 간단히 그리기
        const canvas = document.getElementById('waveformCanvas');
        const ctx = canvas.getContext('2d');
        const width = canvas.width = canvas.clientWidth;
        const height = canvas.height = 100;

        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, width, height);

        const dataView = new DataView(wavData.buffer);
        const samples = [];
        for (let i = 44; i < wavData.length; i += 2) {
            const sample = dataView.getInt16(i, true) / 32768.0;
            samples.push(sample);
        }

        const step = Math.floor(samples.length / width);
        ctx.strokeStyle = '#4CAF50';
        ctx.lineWidth = 1;
        ctx.beginPath();

        for (let i = 0; i < width; i++) {
            const sampleIndex = i * step;
            const sample = samples[sampleIndex] || 0;
            const y = (sample * 0.5 + 0.5) * height;
            if (i === 0) {
                ctx.moveTo(i, y);
            } else {
                ctx.lineTo(i, y);
            }
        }
        ctx.stroke();
    }

    async playOriginal() {
        try {
            await this.player.playWavData(this.originalAudio);
        } catch (error) {
            console.error('재생 실패:', error);
            alert('재생 실패: ' + error.message);
        }
    }

    downloadOriginal() {
        this.player.downloadWav(this.originalAudio, 'original.wav');
    }

    async analyzeVoice() {
        // WAV 데이터를 Float32Array로 변환
        const float32Data = this.wavToFloat32(this.currentAudioData);

        // WASM 메모리에 복사
        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        // C++에서 직접 Canvas에 그리기
        this.module.drawCombinedAnalysis(dataPtr, float32Data.length, this.sampleRate, 'analysisCanvas');

        this.module._free(dataPtr);

        // Calculate max time
        this.audioMaxTime = float32Data.length / this.sampleRate;
    }

    setPitchQuality(quality) {
        try {
            this.module.setPitchShiftQuality(quality);
            const currentQualityName = this.module.getPitchShiftQuality();
            document.getElementById('currentQuality').textContent = `현재: ${currentQualityName}`;
            console.log(`Pitch quality set to: ${quality} (${currentQualityName})`);
        } catch (error) {
            console.error('Failed to set pitch quality:', error);
        }
    }

    async applyPitchShift() {
        console.log('applyPitchShift called');

        try {
            const semitones = parseFloat(document.getElementById('pitchShift').value);
            console.log('Pitch shift semitones:', semitones);

            if (!this.currentAudioData) {
                alert('먼저 오디오를 녹음하거나 업로드하세요.');
                return;
            }

            const float32Data = this.wavToFloat32(this.currentAudioData);
            console.log('Input audio samples:', float32Data.length);

            const dataPtr = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr / 4);

            const result = this.module.applyPitchShift(dataPtr, float32Data.length, this.sampleRate, semitones);
            console.log('Pitch shift result:', result);

            this.module._free(dataPtr);

            this.processedAudio = this.float32ToWav(new Float32Array(result));
            this.currentAudioData = this.processedAudio;
            console.log('Processed audio created, size:', this.processedAudio.length);

            document.getElementById('playProcessed').disabled = false;
            document.getElementById('downloadProcessed').disabled = false;

            this.drawWaveform(this.processedAudio);
            console.log('Pitch shift completed successfully');
        } catch (error) {
            console.error('Pitch shift 실패:', error);
            alert('Pitch shift 실패: ' + error.message);
        }
    }

    async applyTimeStretch() {
        const ratio = parseFloat(document.getElementById('timeStretch').value);
        const float32Data = this.wavToFloat32(this.currentAudioData);

        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        const result = this.module.applyTimeStretch(dataPtr, float32Data.length, this.sampleRate, ratio);
        this.module._free(dataPtr);

        this.processedAudio = this.float32ToWav(new Float32Array(result));
        this.currentAudioData = this.processedAudio;

        document.getElementById('playProcessed').disabled = false;
        document.getElementById('downloadProcessed').disabled = false;

        this.drawWaveform(this.processedAudio);
    }

    async applyFilter() {
        const filterType = parseInt(document.getElementById('filterType').value);
        const param1 = parseFloat(document.getElementById('filterParam1').value);
        const param2 = parseFloat(document.getElementById('filterParam2').value);
        const float32Data = this.wavToFloat32(this.currentAudioData);

        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        const result = this.module.applyVoiceFilter(dataPtr, float32Data.length, this.sampleRate, filterType, param1, param2);
        this.module._free(dataPtr);

        this.processedAudio = this.float32ToWav(new Float32Array(result));
        this.currentAudioData = this.processedAudio;

        document.getElementById('playProcessed').disabled = false;
        document.getElementById('downloadProcessed').disabled = false;

        this.drawWaveform(this.processedAudio);
    }

    async playProcessed() {
        console.log('playProcessed called');
        console.log('processedAudio:', this.processedAudio);

        try {
            if (!this.processedAudio) {
                console.error('processedAudio is null or undefined');
                alert('먼저 음성 효과를 적용해주세요.');
                return;
            }
            console.log('Playing processed audio, size:', this.processedAudio.length);
            await this.player.playWavData(this.processedAudio);
            console.log('Playback completed');
        } catch (error) {
            console.error('재생 실패:', error);
            alert('재생 실패: ' + error.message);
        }
    }

    downloadProcessed() {
        if (!this.processedAudio) {
            alert('먼저 음성 효과를 적용해주세요.');
            return;
        }
        this.player.downloadWav(this.processedAudio, 'processed.wav');
    }

    reset() {
        location.reload();
    }

    // 유틸리티 함수
    wavToFloat32(wavData) {
        const dataView = new DataView(wavData.buffer);
        const samples = [];

        for (let i = 44; i < wavData.length; i += 2) {
            const sample = dataView.getInt16(i, true) / 32768.0;
            samples.push(sample);
        }

        return new Float32Array(samples);
    }

    float32ToWav(float32Data) {
        // 간단한 WAV 헤더 + PCM 데이터
        const wavHeader = new Uint8Array(44);
        const view = new DataView(wavHeader.buffer);

        // RIFF 헤더
        view.setUint32(0, 0x52494646, false); // "RIFF"
        view.setUint32(4, 36 + float32Data.length * 2, true);
        view.setUint32(8, 0x57415645, false); // "WAVE"

        // fmt 청크
        view.setUint32(12, 0x666d7420, false); // "fmt "
        view.setUint32(16, 16, true); // fmt 크기
        view.setUint16(20, 1, true); // PCM
        view.setUint16(22, 1, true); // 모노
        view.setUint32(24, this.sampleRate, true);
        view.setUint32(28, this.sampleRate * 2, true);
        view.setUint16(32, 2, true);
        view.setUint16(34, 16, true);

        // data 청크
        view.setUint32(36, 0x64617461, false); // "data"
        view.setUint32(40, float32Data.length * 2, true);

        // PCM 데이터
        const pcmData = new Int16Array(float32Data.length);
        for (let i = 0; i < float32Data.length; i++) {
            pcmData[i] = Math.max(-1, Math.min(1, float32Data[i])) * 32767;
        }

        // 합치기
        const result = new Uint8Array(44 + pcmData.length * 2);
        result.set(wavHeader);
        result.set(new Uint8Array(pcmData.buffer), 44);

        return result;
    }
}
