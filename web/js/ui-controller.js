import { AudioRecorder } from './audio-recorder.js';
import { AudioPlayer } from './audio-player.js';
import { WhisperController } from './whisper-controller.js';
import { TimelineRenderer } from './timeline-renderer.js';
import { AudioTrimmer } from './audio-trimmer.js';

export class UIController {
    constructor() {
        this.recorder = null;
        this.player = new AudioPlayer();
        this.whisper = new WhisperController();
        this.timeline = new TimelineRenderer('timelineCanvas');
        this.trimmer = null; // Will be initialized after module loads
        this.module = null;
        this.originalAudio = null;
        this.processedAudio = null;
        this.currentAudioData = null;
        this.sampleRate = 48000; // 브라우저 기본값
        this.transcriptionWords = [];
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
        this.trimmer = new AudioTrimmer('analysisCanvas', this.module, this);
        this.module.init();
        this.setupEventListeners();
    }

    setupEventListeners() {
        // 녹음 버튼
        document.getElementById('startRecord').addEventListener('click', () => this.startRecording());
        document.getElementById('stopRecord').addEventListener('click', () => this.stopRecording());
        document.getElementById('uploadFile').addEventListener('click', () => this.uploadFile());
        document.getElementById('fileInput').addEventListener('change', (e) => this.handleFileUpload(e));
        document.getElementById('playOriginal').addEventListener('click', () => this.playOriginal());
        document.getElementById('downloadOriginal').addEventListener('click', () => this.downloadOriginal());

        // 분석 버튼
        document.getElementById('analyzeVoice').addEventListener('click', () => this.analyzeVoice());
        document.getElementById('loadWhisperModel').addEventListener('click', () => this.loadWhisperModel());
        document.getElementById('transcribeVoice').addEventListener('click', () => this.transcribeVoice());
        document.getElementById('trimAudio').addEventListener('click', () => this.trimAudio());
        document.getElementById('resetTrim').addEventListener('click', () => this.resetTrim());

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
        document.getElementById('filterParam1').addEventListener('input', (e) => {
            document.getElementById('param1Value').textContent = e.target.value;
        });
        document.getElementById('filterParam2').addEventListener('input', (e) => {
            document.getElementById('param2Value').textContent = e.target.value;
        });
    }

    async startRecording() {
        document.getElementById('recordStatus').textContent = '녹음 중...';
        document.getElementById('startRecord').disabled = true;
        document.getElementById('stopRecord').disabled = false;

        try {
            await this.recorder.startRecording();
        } catch (error) {
            console.error('녹음 시작 실패:', error);
            document.getElementById('recordStatus').textContent = '녹음 시작 실패: ' + error.message;
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
        document.getElementById('loadWhisperModel').disabled = false;
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
            document.getElementById('loadWhisperModel').disabled = false;
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

        // Calculate max time and enable trimmer
        this.audioMaxTime = float32Data.length / this.sampleRate;
        this.trimmer.enable(this.audioMaxTime);
        this.trimmer.updateStatus();
        document.getElementById('trimAudio').disabled = false;
        document.getElementById('resetTrim').disabled = false;

        // If we have transcription data, update timeline
        if (this.transcriptionWords.length > 0) {
            this.timeline.setWords(this.transcriptionWords, this.audioMaxTime);
            this.timeline.render();
        }
    }

    async loadWhisperModel() {
        const sttStatus = document.getElementById('sttStatus');
        const progressContainer = document.getElementById('modelLoadProgress');
        const progressBar = document.getElementById('progressBar');
        const progressText = document.getElementById('progressText');
        const loadButton = document.getElementById('loadWhisperModel');
        const transcribeButton = document.getElementById('transcribeVoice');

        try {
            loadButton.disabled = true;
            progressContainer.style.display = 'block';
            progressBar.style.width = '0%';
            sttStatus.textContent = 'Whisper 모듈 초기화 중...';

            // Initialize Whisper if not already done
            if (!this.whisper.whisperModule) {
                await this.whisper.init();
            }

            // Load model with progress
            const modelUrl = 'https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin';
            sttStatus.textContent = '모델 다운로드 중...';

            await this.whisper.loadModel(modelUrl, (progress) => {
                progressBar.style.width = progress.percent.toFixed(1) + '%';
                const loadedMB = (progress.loaded / 1024 / 1024).toFixed(1);
                const totalMB = (progress.total / 1024 / 1024).toFixed(1);
                progressText.textContent = `${loadedMB}MB / ${totalMB}MB (${progress.percent.toFixed(1)}%)`;

                if (progress.status) {
                    sttStatus.textContent = progress.status;
                }
            });

            sttStatus.textContent = '모델 로드 완료!';
            progressText.textContent = '모델이 준비되었습니다.';
            transcribeButton.disabled = false;

            setTimeout(() => {
                progressContainer.style.display = 'none';
            }, 2000);

        } catch (error) {
            console.error('모델 로드 실패:', error);
            sttStatus.textContent = '모델 로드 실패: ' + error.message;
            loadButton.disabled = false;
            progressContainer.style.display = 'none';
        }
    }

    async transcribeVoice() {
        const sttStatus = document.getElementById('sttStatus');

        if (!this.whisper.modelLoaded) {
            sttStatus.textContent = '먼저 STT 모델을 로드해주세요.';
            return;
        }

        try {
            sttStatus.textContent = 'STT 변환 중...';
            document.getElementById('transcribeVoice').disabled = true;

            // Convert WAV to Float32
            const float32Data = this.wavToFloat32(this.currentAudioData);

            // Transcribe
            const words = await this.whisper.transcribe(float32Data, this.sampleRate, 'ko');

            // Calculate max time
            const maxTime = float32Data.length / this.sampleRate;

            this.transcriptionWords = words;

            sttStatus.textContent = `STT 완료! (${words.length} 단어)`;
            document.getElementById('transcribeVoice').disabled = false;

            // Show timeline
            document.getElementById('timelineContainer').style.display = 'block';

            // Render timeline
            this.timeline.setWords(words, maxTime);
            this.timeline.render();

            console.log('Transcription words:', words);
        } catch (error) {
            console.error('STT 변환 실패:', error);
            sttStatus.textContent = 'STT 변환 실패: ' + error.message;
            document.getElementById('transcribeVoice').disabled = false;
        }
    }

    async applyPitchShift() {
        const semitones = parseFloat(document.getElementById('pitchShift').value);
        const float32Data = this.wavToFloat32(this.currentAudioData);

        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        const result = this.module.applyPitchShift(dataPtr, float32Data.length, this.sampleRate, semitones);
        this.module._free(dataPtr);

        this.processedAudio = this.float32ToWav(new Float32Array(result));
        this.currentAudioData = this.processedAudio;

        document.getElementById('playProcessed').disabled = false;
        document.getElementById('downloadProcessed').disabled = false;

        this.drawWaveform(this.processedAudio);
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
        await this.player.playWavData(this.processedAudio);
    }

    downloadProcessed() {
        this.player.downloadWav(this.processedAudio, 'processed.wav');
    }

    trimAudio() {
        const range = this.trimmer.getTrimRange();
        const float32Data = this.wavToFloat32(this.currentAudioData);

        // Calculate sample indices
        const startSample = Math.floor(range.start * this.sampleRate);
        const endSample = Math.floor(range.end * this.sampleRate);

        // Extract the selected region
        const trimmedData = float32Data.slice(startSample, endSample);

        // Convert back to WAV
        const trimmedWav = this.float32ToWav(trimmedData);

        // Update current audio
        this.currentAudioData = trimmedWav;
        this.processedAudio = trimmedWav;

        // Update UI
        document.getElementById('playProcessed').disabled = false;
        document.getElementById('downloadProcessed').disabled = false;
        document.getElementById('trimStatus').textContent = `자르기 완료! 새 길이: ${(trimmedData.length / this.sampleRate).toFixed(2)}s`;

        // Disable trimmer and re-analyze
        this.trimmer.disable();
        this.analyzeVoice();

        // Redraw waveform
        this.drawWaveform(this.currentAudioData);
    }

    resetTrim() {
        this.trimmer.reset();
        this.trimmer.updateStatus();
        document.getElementById('trimStatus').textContent = '자르기 영역이 초기화되었습니다.';
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
