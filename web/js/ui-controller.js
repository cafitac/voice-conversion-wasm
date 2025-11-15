import { AudioRecorder } from './audio-recorder.js';
import { AudioPlayer } from './audio-player.js';
import { InteractiveEditor } from './interactive-editor.js';
import { UnifiedEditor } from './unified-editor.js';
import { convertPipelineResultToFloat32Array } from './audio-utils.js';

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

        // Interactive editors
        this.editorHighQuality = null;
        this.editorExternal = null;
        this.currentTab = 'highquality';
        this.frameData = null;
        this.resultHighQuality = null;
        this.resultExternal = null;

        // 통합 에디터 (새로운 UI)
        this.unifiedEditor = null;
        this.sampleAudio = null;
        this.samplePlayer = new AudioPlayer();

        // 벤치마크 리포트 데이터
        this.benchmarkReport = null;
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

        // Interactive editors 초기화
        this.editorHighQuality = new InteractiveEditor('chart-hq');
        this.editorExternal = new InteractiveEditor('chart-ext');

        // 통합 에디터 초기화 (Module 전달)
        this.unifiedEditor = new UnifiedEditor('unified-chart', this.module);

        // 전역 등록 (region 삭제용)
        window.editor_chart_hq = this.editorHighQuality;
        window.editor_chart_ext = this.editorExternal;
        window.unifiedEditor = this.unifiedEditor;

        this.setupEventListeners();

        // 전체 화면 편집에서 돌아온 경우 편집 결과 복원
        this.restoreEditResults();

        // 초기 Pitch 품질 설정 (기본값: external)
        this.setPitchQuality('external');
        // 초기 TimeStretch 품질 설정 (기본값: external)
        this.setTimeStretchQuality('external');
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

        // 탭 전환 (레거시 - 조건부)
        if (document.getElementById('tab-highquality')) {
            document.getElementById('tab-highquality').addEventListener('click', () => this.switchTab('highquality'));
            document.getElementById('tab-external').addEventListener('click', () => this.switchTab('external'));
            document.getElementById('tab-compare').addEventListener('click', () => this.switchTab('compare'));
        }

        // HighQuality 파이프라인 버튼 (레거시 - 조건부)
        if (document.getElementById('analyze-hq')) {
            document.getElementById('analyze-hq').addEventListener('click', () => this.analyzeHighQuality());
            document.getElementById('edit-fullscreen-hq').addEventListener('click', () => this.openFullscreenEditor('highquality'));
            document.getElementById('apply-hq').addEventListener('click', () => this.applyEditsHighQuality());
            document.getElementById('reset-hq').addEventListener('click', () => this.resetHighQuality());
        }

        // External 파이프라인 버튼 (레거시 - 조건부)
        if (document.getElementById('analyze-ext')) {
            document.getElementById('analyze-ext').addEventListener('click', () => this.analyzeExternal());
            document.getElementById('edit-fullscreen-ext').addEventListener('click', () => this.openFullscreenEditor('external'));
            document.getElementById('apply-ext').addEventListener('click', () => this.applyEditsExternal());
            document.getElementById('reset-ext').addEventListener('click', () => this.resetExternal());
        }

        // 비교 버튼 (레거시 - 조건부)
        if (document.getElementById('compare-run')) {
            document.getElementById('compare-run').addEventListener('click', () => this.runComparison());
            document.getElementById('play-compare-hq').addEventListener('click', () => this.playCompareHighQuality());
            document.getElementById('play-compare-ext').addEventListener('click', () => this.playCompareExternal());
        }

        // 리포트 다운로드 버튼 (레거시 - 조건부)
        if (document.getElementById('download-report-json')) {
            document.getElementById('download-report-json').addEventListener('click', () => this.downloadReportJSON());
            document.getElementById('download-report-html').addEventListener('click', () => this.downloadReportHTML());
        }

        // 효과 버튼 (통합)
        document.getElementById('applyAllEffects').addEventListener('click', () => this.applyAllEffects());

        // 초기화 버튼
        document.getElementById('resetPitch').addEventListener('click', () => this.resetPitch());
        document.getElementById('resetTimeStretch').addEventListener('click', () => this.resetTimeStretch());
        document.getElementById('resetFilter').addEventListener('click', () => this.resetFilter());

        // 통합 에디터 버튼
        if (document.getElementById("analyze-unified")) {
            document.getElementById("analyze-unified").addEventListener("click", () => this.analyzeUnified());
            document.getElementById("reset-unified").addEventListener("click", () => this.resetUnified());
            document.getElementById("generate-sample").addEventListener("click", () => this.generateSample());
            document.getElementById("play-sample").addEventListener("click", () => this.playSample());
            document.getElementById("stop-sample").addEventListener("click", () => this.stopSample());
            document.getElementById("download-sample").addEventListener("click", () => this.downloadSample());
            document.getElementById("processing-order").addEventListener("change", () => this.updateProcessingDescription());
        }

        // 재생 및 다운로드
        document.getElementById('playProcessed').addEventListener('click', () => this.playProcessed());
        document.getElementById('downloadProcessed').addEventListener('click', () => this.downloadProcessed());
        document.getElementById('reset').addEventListener('click', () => this.reset());

        // 슬라이더 값 업데이트
        const pitchSlider = document.getElementById('pitchShift');
        pitchSlider.addEventListener('input', (e) => {
            document.getElementById('pitchValue').textContent = e.target.value;
            this.updateSliderBackground(e.target);
            this.updateResetButtons();
        });
        this.updateSliderBackground(pitchSlider);

        const timeSlider = document.getElementById('timeStretch');
        timeSlider.addEventListener('input', (e) => {
            document.getElementById('timeValue').textContent = e.target.value;
            this.updateSliderBackground(e.target);
            this.updateResetButtons();
        });
        this.updateSliderBackground(timeSlider);

        // Pitch 품질 선택
        document.getElementById('pitchQuality').addEventListener('change', (e) => {
            this.setPitchQuality(e.target.value);
        });

        // TimeStretch 품질 선택
        document.getElementById('timeStretchQuality').addEventListener('change', (e) => {
            this.setTimeStretchQuality(e.target.value);
        });

        const filterParam1 = document.getElementById('filterParam1');
        filterParam1.addEventListener('input', (e) => {
            document.getElementById('param1Value').textContent = e.target.value;
            this.updateSliderBackground(e.target);
            this.updateResetButtons();
        });
        this.updateSliderBackground(filterParam1);

        const filterParam2 = document.getElementById('filterParam2');
        filterParam2.addEventListener('input', (e) => {
            document.getElementById('param2Value').textContent = e.target.value;
            this.updateSliderBackground(e.target);
            this.updateResetButtons();
        });
        this.updateSliderBackground(filterParam2);

        // 필터 타입 변경 감지
        document.getElementById('filterType').addEventListener('change', () => {
            this.updateResetButtons();
        });

        // 초기 상태 설정
        this.updateResetButtons();
        this.updateEffectsSectionState();
    }

    /**
     * 음성 효과 섹션 활성화/비활성화
     */
    updateEffectsSectionState() {
        const hasAudio = !!this.originalAudio;
        const effectsNotice = document.getElementById('effectsNotice');
        const effectsContent = document.getElementById('effectsContent');

        if (hasAudio) {
            // 오디오가 있으면: 경고 문구 숨기고, 효과 컨텐츠 표시
            effectsNotice.style.display = 'none';
            effectsContent.style.display = 'block';
            // 초기화 버튼 상태 업데이트 (오디오 상태와 값에 따라)
            this.updateResetButtons();
        } else {
            // 오디오가 없으면: 경고 문구만 표시하고, 효과 컨텐츠 숨김
            effectsNotice.style.display = 'block';
            effectsContent.style.display = 'none';
        }
    }

    async startRecording() {
        document.getElementById('recordStatus').textContent = '마이크 권한 요청 중...';
        document.getElementById('startRecord').disabled = true;
        document.getElementById('stopRecord').disabled = false;

        // 기존 오디오 데이터 및 파형 지우기
        this.originalAudio = null;
        this.currentAudioData = null;
        this.clearWaveform();

        // 관련 버튼 비활성화
        document.getElementById('playOriginal').disabled = true;
        document.getElementById('downloadOriginal').disabled = true;

        // 음성 효과 섹션 비활성화
        this.updateEffectsSectionState();

        // Interactive editor analyze 버튼 비활성화 (조건부)
        if (document.getElementById('analyze-hq')) {
            document.getElementById('analyze-hq').disabled = true;
        }
        if (document.getElementById('analyze-ext')) {
            document.getElementById('analyze-ext').disabled = true;
        }
        if (document.getElementById('compare-run')) {
            document.getElementById('compare-run').disabled = true;
        }

        // 통합 에디터 분석 버튼 비활성화
        if (document.getElementById('analyze-unified')) {
            document.getElementById('analyze-unified').disabled = true;
        }

        try {
            await this.recorder.startRecording();
            const recordStatusEl = document.getElementById('recordStatus');
            recordStatusEl.textContent = '녹음 중...';
            recordStatusEl.classList.add('recording');
        } catch (error) {
            console.error('녹음 시작 실패:', error);

            // 버튼 상태 복원
            document.getElementById('startRecord').disabled = false;
            document.getElementById('stopRecord').disabled = true;

            // 에러 메시지 표시
            const recordStatusEl = document.getElementById('recordStatus');
            const errorMsg = '녹음 시작 실패: ' + error.message;
            recordStatusEl.textContent = errorMsg;
            recordStatusEl.style.color = '#f44336';
            recordStatusEl.classList.remove('recording');

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

        const recordStatusEl = document.getElementById('recordStatus');
        recordStatusEl.textContent = '녹음 완료!';
        recordStatusEl.classList.remove('recording');
        document.getElementById('startRecord').disabled = false;
        document.getElementById('stopRecord').disabled = true;
        document.getElementById('playOriginal').disabled = false;
        document.getElementById('downloadOriginal').disabled = false;

        // 음성 효과 섹션 활성화
        this.updateEffectsSectionState();

        // Interactive editor analyze 버튼 활성화 (조건부)
        if (document.getElementById('analyze-hq')) {
            document.getElementById('analyze-hq').disabled = false;
        }
        if (document.getElementById('analyze-ext')) {
            document.getElementById('analyze-ext').disabled = false;
        }
        if (document.getElementById('compare-run')) {
            document.getElementById('compare-run').disabled = false;
        }

        // 통합 에디터 분석 버튼 활성화
        if (document.getElementById('analyze-unified')) {
            document.getElementById('analyze-unified').disabled = false;
        }

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

            // Bits per sample 추출 (offset 34, 2 bytes)
            const bitsPerSample = view.getUint16(34, true);

            // 채널 수 추출 (offset 22, 2 bytes)
            const numChannels = view.getUint16(22, true);

            // PCM 데이터 시작 위치 찾기 (일반적으로 44바이트 이후)
            let dataOffset = 44;

            // PCM 데이터 추출
            const pcmData = wavData.slice(dataOffset);

            // Float32Array로 변환
            let float32Data;
            if (bitsPerSample === 16) {
                const int16Data = new Int16Array(pcmData.buffer, pcmData.byteOffset, pcmData.byteLength / 2);
                float32Data = new Float32Array(int16Data.length);
                for (let i = 0; i < int16Data.length; i++) {
                    float32Data[i] = int16Data[i] / 32768.0;
                }
            } else if (bitsPerSample === 32) {
                float32Data = new Float32Array(pcmData.buffer, pcmData.byteOffset, pcmData.byteLength / 4);
            } else {
                throw new Error(`지원하지 않는 비트 깊이입니다: ${bitsPerSample}bit`);
            }

            // 스테레오를 모노로 변환
            if (numChannels === 2) {
                const monoData = new Float32Array(float32Data.length / 2);
                for (let i = 0; i < monoData.length; i++) {
                    monoData[i] = (float32Data[i * 2] + float32Data[i * 2 + 1]) / 2;
                }
                float32Data = monoData;
            }

            this.originalAudio = float32Data;
            this.currentAudioData = float32Data;

            document.getElementById('recordStatus').textContent = `파일 업로드 완료! (${file.name}, ${this.sampleRate}Hz)`;
            document.getElementById('playOriginal').disabled = false;
            document.getElementById('downloadOriginal').disabled = false;

            // 음성 효과 섹션 활성화
            this.updateEffectsSectionState();

            // Interactive editor analyze 버튼 활성화 (조건부)
            if (document.getElementById('analyze-hq')) {
                document.getElementById('analyze-hq').disabled = false;
            }
            if (document.getElementById('analyze-ext')) {
                document.getElementById('analyze-ext').disabled = false;
            }
            if (document.getElementById('compare-run')) {
                document.getElementById('compare-run').disabled = false;
            }

            // 통합 에디터 분석 버튼 활성화
            if (document.getElementById('analyze-unified')) {
                document.getElementById('analyze-unified').disabled = false;
            }

            // 파형 그리기
            this.drawWaveform(this.originalAudio);
        } catch (error) {
            console.error('파일 업로드 실패:', error);
            document.getElementById('recordStatus').textContent = '파일 업로드 실패: ' + error.message;
        }
    }

    clearWaveform() {
        // Canvas를 배경색으로 지우기
        const canvas = document.getElementById('waveformCanvas');
        if (!canvas) return;

        const ctx = canvas.getContext('2d');
        const width = canvas.width = canvas.clientWidth;
        const height = canvas.height = 100;

        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, width, height);
    }

    drawWaveform(audioData) {
        // 오디오 데이터를 Canvas에 그리기
        const canvas = document.getElementById('waveformCanvas');
        const ctx = canvas.getContext('2d');
        const width = canvas.width = canvas.clientWidth;
        const height = canvas.height = 100;

        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, width, height);

        let samples;
        if (audioData instanceof Float32Array) {
            // Float32Array는 그대로 사용
            samples = Array.from(audioData);
        } else {
            // WAV 데이터 (Uint8Array)에서 샘플 추출
            const dataView = new DataView(audioData.buffer);
            samples = [];
            for (let i = 44; i < audioData.length; i += 2) {
                const sample = dataView.getInt16(i, true) / 32768.0;
                samples.push(sample);
            }
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
            if (this.originalAudio instanceof Float32Array) {
                await this.player.playFloat32Array(this.originalAudio, this.sampleRate);
            } else {
                await this.player.playWavData(this.originalAudio);
            }
        } catch (error) {
            console.error('재생 실패:', error);
            alert('재생 실패: ' + error.message);
        }
    }

    downloadOriginal() {
        if (this.originalAudio instanceof Float32Array) {
            // Float32Array를 WAV로 변환
            const wavData = this.float32ToWav(this.originalAudio);
            this.player.downloadWav(wavData, 'original.wav');
        } else {
            this.player.downloadWav(this.originalAudio, 'original.wav');
        }
    }

    /**
     * Pitch quality 설정 (새 Pipeline 아키텍처)
     * Quality를 알고리즘 이름으로 변환
     */
    setPitchQuality(quality) {
        try {
            // Quality를 알고리즘 이름으로 매핑
            const algorithmMap = {
                'fast': 'psola',
                'high': 'phase-vocoder',
                'external': 'soundtouch'
            };
            this.currentPitchAlgorithm = algorithmMap[quality] || 'phase-vocoder';

            const displayNames = {
                'psola': 'PSOLA (Fast)',
                'phase-vocoder': 'Phase Vocoder (High Quality)',
                'soundtouch': 'SoundTouch (External)'
            };

            document.getElementById('currentQuality').textContent =
                `현재: ${displayNames[this.currentPitchAlgorithm]}`;
            console.log(`Pitch algorithm set to: ${this.currentPitchAlgorithm}`);
        } catch (error) {
            console.error('Failed to set pitch quality:', error);
        }
    }

    /**
     * TimeStretch quality 설정 (새 Pipeline 아키텍처)
     * Quality를 알고리즘 이름으로 변환
     */
    setTimeStretchQuality(quality) {
        try {
            // Quality를 알고리즘 이름으로 매핑
            const algorithmMap = {
                'fast': 'wsola',
                'high': 'soundtouch',
                'phase-vocoder': 'soundtouch',
                'rubberband': 'rubberband',
                'external': 'soundtouch'
            };
            this.currentDurationAlgorithm = algorithmMap[quality] || 'soundtouch';

            const displayNames = {
                'wsola': 'WSOLA (Fast)',
                'soundtouch': 'SoundTouch',
                'rubberband': 'RubberBand (High Quality)'
            };

            document.getElementById('currentTimeStretchQuality').textContent =
                `현재: ${displayNames[this.currentDurationAlgorithm]}`;
            console.log(`Duration algorithm set to: ${this.currentDurationAlgorithm}`);
        } catch (error) {
            console.error('Failed to set timestretch quality:', error);
        }
    }

    /**
     * 모든 효과를 한 번에 적용 (Pitch + Time Stretch + Filter)
     */
    async applyAllEffects() {
        if (!this.originalAudio) {
            alert('먼저 오디오를 녹음하거나 업로드하세요.');
            return;
        }

        try {
            console.log('applyAllEffects: 모든 효과 적용 시작');

            // 원본 오디오로 시작
            let audioData = this.originalAudio instanceof Float32Array
                ? this.originalAudio
                : this.wavToFloat32(this.originalAudio);

            // 1. Pitch Shift 적용 (값이 0이 아니면)
            const semitones = parseFloat(document.getElementById('pitchShift').value);
            if (Math.abs(semitones) > 0.01) {
                console.log(`✓ Pitch Shift 적용: ${semitones} semitones`);
                audioData = await this.applyPitchShiftInternal(audioData, semitones);
            }

            // 2. Time Stretch 적용 (값이 1.0이 아니면)
            const speed = parseFloat(document.getElementById('timeStretch').value);
            if (Math.abs(speed - 1.0) > 0.01) {
                console.log(`✓ Time Stretch 적용: ${speed}x`);
                audioData = await this.applyTimeStretchInternal(audioData, speed);
            }

            // 3. Filter 적용 (none이 아니면)
            const filterType = document.getElementById('filterType').value;
            if (filterType !== 'none') {
                console.log(`✓ Filter 적용: ${filterType}`);
                audioData = await this.applyFilterInternal(audioData, parseInt(filterType));
            }

            // 최종 결과 저장
            this.processedAudio = audioData;
            this.currentAudioData = audioData;

            document.getElementById('playProcessed').disabled = false;
            document.getElementById('downloadProcessed').disabled = false;

            this.drawWaveform(this.processedAudio);
            console.log('✓ 모든 효과 적용 완료');
        } catch (error) {
            console.error('효과 적용 실패:', error);
            alert('효과 적용 실패: ' + error.message);
        }
    }

    /**
     * Pitch Shift 내부 함수 (헬퍼)
     */
    async applyPitchShiftInternal(audioData, semitones) {
        const float32Data = audioData instanceof Float32Array
            ? audioData
            : this.wavToFloat32(audioData);

        const duration = float32Data.length / this.sampleRate;

        // 전체 오디오에 일정한 pitch shift를 위한 edit points 생성
        const editPoints = [
            { time: 0, semitones: semitones },
            { time: duration, semitones: semitones }
        ];

        // 1단계: 전처리 + 보간
        const interpolatedFrames = this.module.preprocessAndInterpolate(
            duration,
            this.sampleRate,
            editPoints,
            3.0,   // gradientThreshold
            0.02   // frameInterval
        );

        // 2단계: Pipeline 처리
        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        const algorithm = this.currentPitchAlgorithm || 'phase-vocoder';

        const resultView = this.module.processAudioWithPipeline(
            dataPtr,
            float32Data.length,
            this.sampleRate,
            interpolatedFrames,
            algorithm,      // Pitch algorithm
            'none',         // No duration processing
            false,          // previewMode
            3.0,            // gradientThreshold
            0.02            // frameInterval
        );

        this.module._free(dataPtr);

        // Float32Array로 변환
        return convertPipelineResultToFloat32Array(resultView);
    }

    /**
     * Time Stretch 내부 함수 (헬퍼)
     */
    async applyTimeStretchInternal(audioData, speed) {
        const float32Data = audioData instanceof Float32Array
            ? audioData
            : this.wavToFloat32(audioData);

        // Speed를 Duration Ratio로 변환
        const ratio = 1.0 / speed;
        const duration = float32Data.length / this.sampleRate;

        // Duration만 변경 (pitch는 변경 안 함)
        const frameInterval = 0.02; // 20ms
        const numFrames = Math.ceil(duration / frameInterval);
        const interpolatedFrames = [];

        for (let i = 0; i < numFrames; i++) {
            interpolatedFrames.push({
                time: i * frameInterval,
                pitchSemitones: 0.0,      // Pitch 변경 없음
                durationRatio: ratio,      // Duration ratio 설정
                isEdited: false,
                isOutlier: false,
                isInterpolated: true
            });
        }

        // Pipeline 처리
        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        const algorithm = this.currentDurationAlgorithm || 'soundtouch';

        const resultView = this.module.processAudioWithPipeline(
            dataPtr,
            float32Data.length,
            this.sampleRate,
            interpolatedFrames,
            'none',         // No pitch processing
            algorithm,      // Duration algorithm
            false,          // previewMode
            3.0,            // gradientThreshold
            0.02            // frameInterval
        );

        this.module._free(dataPtr);

        // Float32Array로 변환
        return convertPipelineResultToFloat32Array(resultView);
    }

    /**
     * Filter 내부 함수 (헬퍼)
     */
    async applyFilterInternal(audioData, filterType) {
        const float32Data = audioData instanceof Float32Array
            ? audioData
            : this.wavToFloat32(audioData);

        const param1 = parseFloat(document.getElementById('filterParam1').value);
        const param2 = parseFloat(document.getElementById('filterParam2').value);

        const dataPtr = this.module._malloc(float32Data.length * 4);
        this.module.HEAPF32.set(float32Data, dataPtr / 4);

        const result = this.module.applyVoiceFilter(dataPtr, float32Data.length, this.sampleRate, filterType, param1, param2);
        this.module._free(dataPtr);

        return new Float32Array(result);
    }

    /**
     * 초기화 버튼 활성화 상태 업데이트
     */
    updateResetButtons() {
        const hasAudio = !!this.originalAudio;

        // Pitch 초기화 버튼: 값이 0이 아니면 활성화 (오디오가 있을 때만)
        const pitchValue = parseFloat(document.getElementById('pitchShift').value);
        document.getElementById('resetPitch').disabled = !hasAudio || Math.abs(pitchValue) < 0.01;

        // Time Stretch 초기화 버튼: 값이 1.0이 아니면 활성화 (오디오가 있을 때만)
        const timeValue = parseFloat(document.getElementById('timeStretch').value);
        document.getElementById('resetTimeStretch').disabled = !hasAudio || Math.abs(timeValue - 1.0) < 0.01;

        // Filter 초기화 버튼: 필터가 "none"이 아니면 활성화 (오디오가 있을 때만)
        const filterType = document.getElementById('filterType').value;
        document.getElementById('resetFilter').disabled = !hasAudio || filterType === 'none';
    }

    /**
     * Pitch 초기화
     */
    resetPitch() {
        const pitchSlider = document.getElementById('pitchShift');
        pitchSlider.value = 0;
        document.getElementById('pitchValue').textContent = '0';
        this.updateSliderBackground(pitchSlider);
        this.updateResetButtons();
    }

    /**
     * Time Stretch 초기화
     */
    resetTimeStretch() {
        const timeSlider = document.getElementById('timeStretch');
        timeSlider.value = 1.0;
        document.getElementById('timeValue').textContent = '1.0';
        this.updateSliderBackground(timeSlider);
        this.updateResetButtons();
    }

    /**
     * Filter 초기화
     */
    resetFilter() {
        document.getElementById('filterType').value = 'none';

        const param1Slider = document.getElementById('filterParam1');
        param1Slider.value = 0.5;
        document.getElementById('param1Value').textContent = '0.5';
        this.updateSliderBackground(param1Slider);

        const param2Slider = document.getElementById('filterParam2');
        param2Slider.value = 0.5;
        document.getElementById('param2Value').textContent = '0.5';
        this.updateSliderBackground(param2Slider);

        this.updateResetButtons();
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

            if (this.processedAudio instanceof Float32Array) {
                await this.player.playFloat32Array(this.processedAudio, this.sampleRate);
            } else {
                await this.player.playWavData(this.processedAudio);
            }
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

        if (this.processedAudio instanceof Float32Array) {
            const wavData = this.float32ToWav(this.processedAudio);
            this.player.downloadWav(wavData, 'processed.wav');
        } else {
            this.player.downloadWav(this.processedAudio, 'processed.wav');
        }
    }

    reset() {
        location.reload();
    }

    // ========== Interactive Editor Methods ==========

    switchTab(tabName) {
        this.currentTab = tabName;

        // 탭 버튼 상태 업데이트
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.remove('active');
        });
        document.getElementById(`tab-${tabName}`).classList.add('active');

        // 뷰 전환
        document.querySelectorAll('.pipeline-view').forEach(view => {
            view.classList.add('hidden');
        });
        document.getElementById(`pipeline-${tabName}`).classList.remove('hidden');
    }

    async analyzeHighQuality() {
        if (!this.currentAudioData) {
            alert('먼저 오디오를 녹음하거나 업로드하세요.');
            return;
        }

        try {
            const float32Data = this.wavToFloat32(this.currentAudioData);
            const dataPtr = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr / 4);

            // getFrameDataArray 호출
            const frameDataArray = this.module.getFrameDataArray(dataPtr, float32Data.length, this.sampleRate);
            this.module._free(dataPtr);

            // JavaScript 배열로 변환
            this.frameData = [];
            for (let i = 0; i < frameDataArray.length; i++) {
                const frame = frameDataArray[i];
                this.frameData.push({
                    frameIndex: frame.frameIndex,
                    time: frame.time,
                    pitch: frame.pitch,
                    rms: frame.rms,
                    isVoice: frame.isVoice
                });
            }

            // Editor에 데이터 설정
            this.editorHighQuality.setFrameData(this.frameData);

            // sessionStorage에 저장 (전체 화면 편집용)
            try {
                const audioBase64 = this.arrayBufferToBase64(this.currentAudioData);
                sessionStorage.setItem('editData', JSON.stringify({
                    pipeline: 'highquality',
                    frames: this.frameData,
                    pitchEdits: this.editorHighQuality.pitchEdits,
                    durationRegions: this.editorHighQuality.durationRegions,
                    audioData: audioBase64,
                    sampleRate: this.sampleRate
                }));
            } catch (error) {
                console.warn('오디오 데이터 저장 실패 (파일이 너무 큼):', error);
            }

            // 버튼 활성화
            document.getElementById('apply-hq').disabled = false;
            document.getElementById('reset-hq').disabled = false;
            document.getElementById('edit-fullscreen-hq').disabled = false;

            // 통계 표시
            this.displayStats('stats-hq', this.frameData);
        } catch (error) {
            console.error('HighQuality 분석 실패:', error);
            alert('분석 실패: ' + error.message);
        }
    }

    async analyzeExternal() {
        if (!this.currentAudioData) {
            alert('먼저 오디오를 녹음하거나 업로드하세요.');
            return;
        }

        try {
            const float32Data = this.wavToFloat32(this.currentAudioData);
            const dataPtr = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr / 4);

            // getFrameDataArray 호출
            const frameDataArray = this.module.getFrameDataArray(dataPtr, float32Data.length, this.sampleRate);
            this.module._free(dataPtr);

            // JavaScript 배열로 변환
            this.frameData = [];
            for (let i = 0; i < frameDataArray.length; i++) {
                const frame = frameDataArray[i];
                this.frameData.push({
                    frameIndex: frame.frameIndex,
                    time: frame.time,
                    pitch: frame.pitch,
                    rms: frame.rms,
                    isVoice: frame.isVoice
                });
            }

            // Editor에 데이터 설정
            this.editorExternal.setFrameData(this.frameData);

            // sessionStorage에 저장 (전체 화면 편집용)
            try {
                const audioBase64 = this.arrayBufferToBase64(this.currentAudioData);
                sessionStorage.setItem('editData', JSON.stringify({
                    pipeline: 'external',
                    frames: this.frameData,
                    pitchEdits: this.editorExternal.pitchEdits,
                    durationRegions: this.editorExternal.durationRegions,
                    audioData: audioBase64,
                    sampleRate: this.sampleRate
                }));
            } catch (error) {
                console.warn('오디오 데이터 저장 실패 (파일이 너무 큼):', error);
            }

            // 버튼 활성화
            document.getElementById('apply-ext').disabled = false;
            document.getElementById('reset-ext').disabled = false;
            document.getElementById('edit-fullscreen-ext').disabled = false;

            // 통계 표시
            this.displayStats('stats-ext', this.frameData);
        } catch (error) {
            console.error('External 분석 실패:', error);
            alert('분석 실패: ' + error.message);
        }
    }

    async applyEditsHighQuality() {
        try {
            const edits = this.editorHighQuality.getEdits();

            console.log('[applyEditsHighQuality] 편집 데이터:', {
                keyPointsCount: edits.keyPoints.length,
                durationRegionsCount: edits.durationRegions.length,
                keyPoints: edits.keyPoints,
                durationRegions: edits.durationRegions
            });

            // Key points에 편집이 있는지 확인
            const hasKeyPointEdits = edits.keyPoints.some(kp => Math.abs(kp.semitones) > 0.01);

            // Pitch만 편집하고 Duration은 편집하지 않은 경우 확인
            if (hasKeyPointEdits && edits.durationRegions.length === 0) {
                console.log('✓ Pitch만 편집됨, Duration은 편집 없음');
            } else if (!hasKeyPointEdits && edits.durationRegions.length > 0) {
                console.log('✓ Duration만 편집됨, Pitch는 편집 없음');
            } else if (hasKeyPointEdits && edits.durationRegions.length > 0) {
                console.log('✓ Pitch와 Duration 둘 다 편집됨');
            } else {
                console.warn('⚠ 편집된 내용이 없습니다!');
                alert('편집된 내용이 없습니다.');
                return;
            }

            const float32Data = this.wavToFloat32(this.currentAudioData);

            const dataPtr = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr / 4);

            // Key points와 duration regions를 C++에 전달
            const keyPointsArray = edits.keyPoints;
            const durationRegionsArray = edits.durationRegions;

            const startTime = performance.now();
            // 새로운 WithKeyPoints 함수 호출
            const result = this.module.applyEditsHighQualityWithKeyPoints(
                dataPtr,
                float32Data.length,
                this.sampleRate,
                keyPointsArray,
                durationRegionsArray
            );
            const endTime = performance.now();

            this.module._free(dataPtr);

            this.resultHighQuality = new Float32Array(result);
            this.processedAudio = this.float32ToWav(this.resultHighQuality);
            this.currentAudioData = this.processedAudio;

            document.getElementById('playProcessed').disabled = false;
            document.getElementById('downloadProcessed').disabled = false;

            this.drawWaveform(this.processedAudio);

            console.log('[applyEditsHighQuality] 처리 완료:', {
                inputSamples: float32Data.length,
                outputSamples: this.resultHighQuality.length,
                processingTime: (endTime - startTime).toFixed(2) + 'ms'
            });

            alert(`편집 적용 완료! (처리 시간: ${(endTime - startTime).toFixed(2)}ms)`);
        } catch (error) {
            console.error('HighQuality 편집 적용 실패:', error);
            alert('편집 적용 실패: ' + error.message);
        }
    }

    async applyEditsExternal() {
        console.log('[applyEditsExternal] 함수 시작');
        console.log('[applyEditsExternal] editorExternal:', this.editorExternal);

        try {
            // 새로운 방식: key points만 C++에 전달
            const edits = this.editorExternal.getEdits();
            const keyPointsArray = edits.keyPoints;
            const durationRegionsArray = edits.durationRegions;

            // Key points에 편집이 있는지 확인
            const hasKeyPointEdits = keyPointsArray.some(kp => Math.abs(kp.semitones) > 0.01);

            console.log('[applyEditsExternal] 편집 데이터:', {
                keyPointsCount: keyPointsArray.length,
                editedKeyPoints: keyPointsArray.filter(kp => Math.abs(kp.semitones) > 0.01).length,
                durationRegionsCount: durationRegionsArray.length,
                sampleKeyPoints: keyPointsArray.slice(0, 5).map(kp => ({ frame: kp.frameIndex, shift: kp.semitones.toFixed(2) }))
            });

            // Pitch만 편집하고 Duration은 편집하지 않은 경우 확인
            if (hasKeyPointEdits && durationRegionsArray.length === 0) {
                console.log('✓ Pitch만 편집됨, Duration은 편집 없음');
            } else if (!hasKeyPointEdits && durationRegionsArray.length > 0) {
                console.log('✓ Duration만 편집됨, Pitch는 편집 없음');
            } else if (hasKeyPointEdits && durationRegionsArray.length > 0) {
                console.log('✓ Pitch와 Duration 둘 다 편집됨');
            } else {
                console.warn('⚠ 편집된 내용이 없습니다!');
                alert('편집된 내용이 없습니다.');
                return;
            }

            const float32Data = this.wavToFloat32(this.currentAudioData);

            const dataPtr = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr / 4);

            const startTime = performance.now();
            // 새로운 WithKeyPoints 함수 호출
            const result = this.module.applyEditsExternalWithKeyPoints(
                dataPtr,
                float32Data.length,
                this.sampleRate,
                keyPointsArray,
                durationRegionsArray
            );
            const endTime = performance.now();

            this.module._free(dataPtr);

            this.resultExternal = new Float32Array(result);
            this.processedAudio = this.float32ToWav(this.resultExternal);
            this.currentAudioData = this.processedAudio;

            document.getElementById('playProcessed').disabled = false;
            document.getElementById('downloadProcessed').disabled = false;

            this.drawWaveform(this.processedAudio);

            console.log('[applyEditsExternal] 처리 완료:', {
                inputSamples: float32Data.length,
                outputSamples: this.resultExternal.length,
                processingTime: (endTime - startTime).toFixed(2) + 'ms'
            });

            alert(`편집 적용 완료! (처리 시간: ${(endTime - startTime).toFixed(2)}ms)`);
        } catch (error) {
            console.error('External 편집 적용 실패:', error);
            alert('편집 적용 실패: ' + error.message);
        }
    }

    resetHighQuality() {
        this.editorHighQuality.reset();
        document.getElementById('apply-hq').disabled = true;
    }

    resetExternal() {
        this.editorExternal.reset();
        document.getElementById('apply-ext').disabled = true;
    }

    async runComparison() {
        if (!this.frameData) {
            alert('먼저 분석을 실행하세요.');
            return;
        }

        try {
            // 동일한 편집을 양쪽에 적용 (예시: HighQuality 편집 사용)
            const edits = this.editorHighQuality.getEdits();
            const float32Data = this.wavToFloat32(this.currentAudioData);

            // HighQuality 처리
            const dataPtr1 = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr1 / 4);
            const startHQ = performance.now();
            const resultHQ = this.module.applyEditsHighQuality(
                dataPtr1,
                float32Data.length,
                this.sampleRate,
                edits.pitchEdits,
                edits.durationRegions
            );
            const endHQ = performance.now();
            this.module._free(dataPtr1);
            this.resultHighQuality = new Float32Array(resultHQ);

            // External 처리
            const dataPtr2 = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, dataPtr2 / 4);
            const startExt = performance.now();
            const resultExt = this.module.applyEditsExternal(
                dataPtr2,
                float32Data.length,
                this.sampleRate,
                edits.pitchEdits,
                edits.durationRegions
            );
            const endExt = performance.now();
            this.module._free(dataPtr2);
            this.resultExternal = new Float32Array(resultExt);

            // 품질 분석
            const originalPtr = this.module._malloc(float32Data.length * 4);
            this.module.HEAPF32.set(float32Data, originalPtr / 4);

            const resultHQPtr = this.module._malloc(this.resultHighQuality.length * 4);
            this.module.HEAPF32.set(this.resultHighQuality, resultHQPtr / 4);

            const resultExtPtr = this.module._malloc(this.resultExternal.length * 4);
            this.module.HEAPF32.set(this.resultExternal, resultExtPtr / 4);

            const qualityHQ = this.module.analyzeQuality(
                originalPtr, float32Data.length,
                resultHQPtr, this.resultHighQuality.length,
                this.sampleRate
            );

            const qualityExt = this.module.analyzeQuality(
                originalPtr, float32Data.length,
                resultExtPtr, this.resultExternal.length,
                this.sampleRate
            );

            this.module._free(originalPtr);
            this.module._free(resultHQPtr);
            this.module._free(resultExtPtr);

            // 벤치마크 리포트 데이터 저장
            this.benchmarkReport = {
                timestamp: new Date().toISOString(),
                sampleRate: this.sampleRate,
                originalLength: float32Data.length,
                highQuality: {
                    processingTime: endHQ - startHQ,
                    outputLength: this.resultHighQuality.length,
                    quality: qualityHQ
                },
                external: {
                    processingTime: endExt - startExt,
                    outputLength: this.resultExternal.length,
                    quality: qualityExt
                },
                edits: edits
            };

            // 비교 차트 그리기
            this.drawComparisonChart('chart-compare-hq', this.resultHighQuality, 'HighQuality');
            this.drawComparisonChart('chart-compare-ext', this.resultExternal, 'External');

            // 리포트 컨트롤 표시
            document.getElementById('report-controls').classList.remove('hidden');

            // 재생 버튼 활성화
            document.getElementById('play-compare-hq').disabled = false;
            document.getElementById('play-compare-ext').disabled = false;

            // 비교 통계 표시
            const statsDiv = document.getElementById('compare-stats');
            statsDiv.classList.remove('hidden');
            statsDiv.innerHTML = `
                <h4>처리 시간 비교</h4>
                <ul>
                    <li><strong>HighQuality:</strong> ${(endHQ - startHQ).toFixed(2)}ms</li>
                    <li><strong>External:</strong> ${(endExt - startExt).toFixed(2)}ms</li>
                    <li><strong>속도 차이:</strong> ${((endExt - startExt) / (endHQ - startHQ)).toFixed(2)}x</li>
                </ul>
                <h4>출력 길이 비교</h4>
                <ul>
                    <li><strong>HighQuality:</strong> ${this.resultHighQuality.length} samples</li>
                    <li><strong>External:</strong> ${this.resultExternal.length} samples</li>
                </ul>
                <h4>품질 메트릭 비교</h4>
                <table style="width: 100%; border-collapse: collapse; margin-top: 10px;">
                    <thead>
                        <tr style="border-bottom: 2px solid var(--border-color);">
                            <th style="padding: 8px; text-align: left; color: var(--text-primary);">메트릭</th>
                            <th style="padding: 8px; text-align: right; color: var(--text-primary);">HighQuality</th>
                            <th style="padding: 8px; text-align: right; color: var(--text-primary);">External</th>
                            <th style="padding: 8px; text-align: center; color: var(--text-primary);">승자</th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr style="border-bottom: 1px solid var(--border-color);">
                            <td style="padding: 8px; color: var(--text-secondary);">SNR (dB)</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityHQ.snr.toFixed(2)}</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityExt.snr.toFixed(2)}</td>
                            <td style="padding: 8px; text-align: center;">${qualityHQ.snr > qualityExt.snr ? '🏆 HQ' : '🏆 Ext'}</td>
                        </tr>
                        <tr style="border-bottom: 1px solid var(--border-color);">
                            <td style="padding: 8px; color: var(--text-secondary);">RMS Error</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityHQ.rmsError.toFixed(4)}</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityExt.rmsError.toFixed(4)}</td>
                            <td style="padding: 8px; text-align: center;">${qualityHQ.rmsError < qualityExt.rmsError ? '🏆 HQ' : '🏆 Ext'}</td>
                        </tr>
                        <tr style="border-bottom: 1px solid var(--border-color);">
                            <td style="padding: 8px; color: var(--text-secondary);">Peak Error</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityHQ.peakError.toFixed(4)}</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityExt.peakError.toFixed(4)}</td>
                            <td style="padding: 8px; text-align: center;">${qualityHQ.peakError < qualityExt.peakError ? '🏆 HQ' : '🏆 Ext'}</td>
                        </tr>
                        <tr style="border-bottom: 1px solid var(--border-color);">
                            <td style="padding: 8px; color: var(--text-secondary);">Spectral Distortion (dB)</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityHQ.spectralDistortion.toFixed(2)}</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityExt.spectralDistortion.toFixed(2)}</td>
                            <td style="padding: 8px; text-align: center;">${qualityHQ.spectralDistortion < qualityExt.spectralDistortion ? '🏆 HQ' : '🏆 Ext'}</td>
                        </tr>
                        <tr>
                            <td style="padding: 8px; color: var(--text-secondary);">Correlation</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityHQ.correlation.toFixed(4)}</td>
                            <td style="padding: 8px; text-align: right; color: var(--text-secondary);">${qualityExt.correlation.toFixed(4)}</td>
                            <td style="padding: 8px; text-align: center;">${qualityHQ.correlation > qualityExt.correlation ? '🏆 HQ' : '🏆 Ext'}</td>
                        </tr>
                    </tbody>
                </table>
            `;

            alert('비교 완료! 각 결과를 재생하고 품질 메트릭을 확인해보세요.');
        } catch (error) {
            console.error('비교 실패:', error);
            alert('비교 실패: ' + error.message);
        }
    }

    /**
     * ArrayBuffer를 Base64로 변환 (청크 방식으로 최적화)
     */
    arrayBufferToBase64(buffer) {
        const bytes = new Uint8Array(buffer);
        const chunkSize = 0x8000; // 32KB chunks
        let binary = '';

        for (let i = 0; i < bytes.length; i += chunkSize) {
            const chunk = bytes.subarray(i, Math.min(i + chunkSize, bytes.length));
            try {
                binary += String.fromCharCode.apply(null, Array.from(chunk));
            } catch (e) {
                // Fallback for very large chunks
                for (let j = 0; j < chunk.length; j++) {
                    binary += String.fromCharCode(chunk[j]);
                }
            }
        }

        return btoa(binary);
    }

    /**
     * Base64를 ArrayBuffer로 변환
     */
    base64ToArrayBuffer(base64) {
        const binary = atob(base64);
        const len = binary.length;
        const bytes = new Uint8Array(len);

        for (let i = 0; i < len; i++) {
            bytes[i] = binary.charCodeAt(i);
        }

        return bytes.buffer;
    }

    /**
     * 전체 화면 편집 페이지 열기
     */
    openFullscreenEditor(pipeline) {
        // 현재 편집기의 최신 데이터 가져오기
        const editor = pipeline === 'highquality' ? this.editorHighQuality : this.editorExternal;

        try {
            // 오디오 데이터를 Base64로 변환
            const audioBase64 = this.currentAudioData ? this.arrayBufferToBase64(this.currentAudioData) : null;

            console.log('[openFullscreenEditor] 데이터 저장 중:', {
                pipeline: pipeline,
                frames: this.frameData.length,
                pitchEdits: editor.pitchEdits.length,
                durationRegions: editor.durationRegions.length
            });

            // sessionStorage에 저장 (전체 pitchEdits 저장)
            sessionStorage.setItem('editData', JSON.stringify({
                pipeline: pipeline,
                frames: this.frameData,
                pitchEdits: editor.pitchEdits,  // 전체 저장
                durationRegions: editor.durationRegions,
                audioData: audioBase64,
                sampleRate: this.sampleRate
            }));

            // 편집 페이지로 이동
            window.location.href = 'editor.html';
        } catch (error) {
            console.error('데이터 저장 실패:', error);
            alert('오디오 파일이 너무 커서 전체 화면 편집을 열 수 없습니다. 더 작은 파일을 사용해주세요.');
        }
    }

    /**
     * 전체 화면 편집에서 돌아왔을 때 편집 결과 복원
     */
    restoreEditResults() {
        const editResults = sessionStorage.getItem('editResults');
        const editData = sessionStorage.getItem('editData');

        console.log('복원 시작 - editResults:', editResults ? 'exists' : 'null');
        console.log('복원 시작 - editData:', editData ? 'exists' : 'null');

        // editData만 있어도 복원 진행 (editResults는 선택사항)
        if (editData) {
            try {
                const results = editResults ? JSON.parse(editResults) : null;
                const data = JSON.parse(editData);

                console.log('데이터 파싱 완료');
                console.log('- pipeline:', data.pipeline);
                console.log('- frames:', data.frames ? data.frames.length : 0);
                console.log('- audioData 존재:', !!data.audioData);
                console.log('- audioData 크기:', data.audioData ? data.audioData.length : 0);

                // 프레임 데이터 복원
                this.frameData = data.frames;

                // 오디오 데이터 복원
                if (data.audioData) {
                    console.log('오디오 데이터 복원 시작...');
                    this.currentAudioData = this.base64ToArrayBuffer(data.audioData);
                    this.sampleRate = data.sampleRate || 48000;

                    // originalAudio도 설정 (재생용)
                    this.originalAudio = new Uint8Array(this.currentAudioData);

                    console.log('오디오 데이터 복원 완료:', this.currentAudioData.byteLength, 'bytes');

                    // 파형 그래프 그리기
                    this.drawWaveform(new Uint8Array(this.currentAudioData));

                    // 녹음 상태 업데이트
                    const audioSeconds = (this.currentAudioData.byteLength - 44) / (this.sampleRate * 2);
                    document.getElementById('recordStatus').textContent = `복원됨 (${audioSeconds.toFixed(1)}초)`;

                    // 원본 오디오 버튼 활성화
                    document.getElementById('playOriginal').disabled = false;
                    document.getElementById('downloadOriginal').disabled = false;
                    document.getElementById('analyze-hq').disabled = false;
                    document.getElementById('analyze-ext').disabled = false;

                    // 효과 버튼 활성화
                    // 음성 효과 섹션 활성화
                    this.updateEffectsSectionState();

                    // 비교 버튼 활성화
                    document.getElementById('compare-run').disabled = false;

                    console.log('모든 버튼 활성화 완료 + 파형 그래프 표시');
                } else {
                    console.warn('audioData가 없습니다!');
                }

                // 파이프라인에 따라 적용
                if (data.pipeline === 'highquality') {
                    console.log('HighQuality 복원 시작...');
                    console.log('- frameData 개수:', this.frameData ? this.frameData.length : 0);
                    console.log('- keyPoints 개수:', results?.keyPoints ? results.keyPoints.length : 0);
                    console.log('- durationRegions 개수:', results?.durationRegions ? results.durationRegions.length : 0);

                    // 프레임 데이터와 편집 데이터를 함께 설정
                    this.editorHighQuality.frameData = this.frameData;

                    // keyPoints 복원 (없으면 초기화)
                    if (results?.keyPoints && results.keyPoints.length > 0) {
                        this.editorHighQuality.keyPoints = results.keyPoints;
                        // Key points를 기반으로 pitchEdits 재계산 (시각화용)
                        this.editorHighQuality.pitchEdits = this.frameData.map((f, i) => ({
                            frameIndex: i,
                            semitones: 0
                        }));
                        this.editorHighQuality.interpolateAllFrames();
                    } else {
                        // keyPoints가 없으면 초기화
                        this.editorHighQuality.pitchEdits = this.frameData.map((f, i) => ({
                            frameIndex: i,
                            semitones: 0
                        }));
                    }

                    this.editorHighQuality.durationRegions = results?.durationRegions || [];

                    // Duration regions 표시 업데이트
                    if (this.editorHighQuality.durationRegions.length > 0) {
                        this.editorHighQuality.updateDurationRegionsDisplay();
                    }

                    // 차트 렌더링
                    console.log('render() 호출 전');
                    this.editorHighQuality.render();
                    console.log('render() 호출 후');

                    console.log('HighQuality 편집 데이터 복원 완료:', {
                        frameData: this.editorHighQuality.frameData.length,
                        keyPoints: this.editorHighQuality.keyPoints.length,
                        durationRegions: this.editorHighQuality.durationRegions.length
                    });

                    // 버튼 활성화
                    document.getElementById('apply-hq').disabled = false;
                    document.getElementById('reset-hq').disabled = false;
                    document.getElementById('edit-fullscreen-hq').disabled = false;

                    // 통계 표시
                    this.displayStats('stats-hq', this.frameData);

                    // HighQuality 탭으로 전환
                    this.switchTab('highquality');
                } else if (data.pipeline === 'external') {
                    console.log('External 복원 시작...');
                    console.log('- frameData 개수:', this.frameData ? this.frameData.length : 0);
                    console.log('- keyPoints 개수:', results?.keyPoints ? results.keyPoints.length : 0);
                    console.log('- durationRegions 개수:', results?.durationRegions ? results.durationRegions.length : 0);

                    // 프레임 데이터와 편집 데이터를 함께 설정
                    this.editorExternal.frameData = this.frameData;

                    // keyPoints 복원 (없으면 초기화)
                    if (results?.keyPoints && results.keyPoints.length > 0) {
                        this.editorExternal.keyPoints = results.keyPoints;
                        // Key points를 기반으로 pitchEdits 재계산 (시각화용)
                        this.editorExternal.pitchEdits = this.frameData.map((f, i) => ({
                            frameIndex: i,
                            semitones: 0
                        }));
                        this.editorExternal.interpolateAllFrames();
                    } else {
                        // keyPoints가 없으면 초기화
                        this.editorExternal.pitchEdits = this.frameData.map((f, i) => ({
                            frameIndex: i,
                            semitones: 0
                        }));
                    }

                    this.editorExternal.durationRegions = results?.durationRegions || [];

                    // Duration regions 표시 업데이트
                    if (this.editorExternal.durationRegions.length > 0) {
                        this.editorExternal.updateDurationRegionsDisplay();
                    }

                    // 차트 렌더링
                    console.log('render() 호출 전');
                    this.editorExternal.render();
                    console.log('render() 호출 후');

                    console.log('External 편집 데이터 복원 완료:', {
                        frameData: this.editorExternal.frameData.length,
                        keyPoints: this.editorExternal.keyPoints.length,
                        durationRegions: this.editorExternal.durationRegions.length
                    });

                    // 버튼 활성화
                    document.getElementById('apply-ext').disabled = false;
                    document.getElementById('reset-ext').disabled = false;
                    document.getElementById('edit-fullscreen-ext').disabled = false;

                    // 통계 표시
                    this.displayStats('stats-ext', this.frameData);

                    // External 탭으로 전환
                    this.switchTab('external');
                }

                // 복원 후 삭제
                sessionStorage.removeItem('editResults');

                console.log('편집 결과 복원 완료');
            } catch (error) {
                console.error('편집 결과 복원 실패:', error);
                sessionStorage.removeItem('editResults');
                sessionStorage.removeItem('editData');
            }
        }
    }

    displayStats(containerId, frameData) {
        const statsDiv = document.getElementById(containerId);
        statsDiv.classList.remove('hidden');

        const voiceFrames = frameData.filter(f => f.isVoice).length;
        const totalFrames = frameData.length;
        const avgPitch = frameData.filter(f => f.isVoice && f.pitch > 0)
            .reduce((sum, f) => sum + f.pitch, 0) / voiceFrames;

        statsDiv.innerHTML = `
            <h4>분석 통계</h4>
            <ul>
                <li><strong>전체 프레임:</strong> ${totalFrames}</li>
                <li><strong>음성 프레임:</strong> ${voiceFrames} (${(voiceFrames / totalFrames * 100).toFixed(1)}%)</li>
                <li><strong>평균 Pitch:</strong> ${avgPitch.toFixed(2)} Hz</li>
            </ul>
        `;
    }

    drawComparisonChart(containerId, audioData, label) {
        const container = document.getElementById(containerId);
        if (!container || !audioData || audioData.length === 0) return;

        // 컨테이너 초기화
        container.innerHTML = '';

        // 크기 설정
        const margin = { top: 20, right: 20, bottom: 30, left: 50 };
        const width = container.clientWidth || 400;
        const height = 250;
        const innerWidth = width - margin.left - margin.right;
        const innerHeight = height - margin.top - margin.bottom;

        // SVG 생성
        const svg = d3.select(container)
            .append('svg')
            .attr('width', width)
            .attr('height', height)
            .style('background', 'var(--bg-primary)')
            .style('border-radius', '12px');

        const g = svg.append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        // 다운샘플링 (너무 많은 포인트는 렌더링 속도 저하)
        const downsampleFactor = Math.ceil(audioData.length / 2000);
        const samples = [];
        for (let i = 0; i < audioData.length; i += downsampleFactor) {
            samples.push({ index: i, value: audioData[i] });
        }

        // Scale 설정
        const xScale = d3.scaleLinear()
            .domain([0, audioData.length - 1])
            .range([0, innerWidth]);

        const yScale = d3.scaleLinear()
            .domain([-1, 1])
            .range([innerHeight, 0]);

        // 축 추가
        g.append('g')
            .attr('transform', `translate(0,${innerHeight})`)
            .call(d3.axisBottom(xScale).ticks(5).tickFormat(d => `${(d / this.sampleRate).toFixed(2)}s`))
            .style('color', 'var(--text-secondary)');

        g.append('g')
            .call(d3.axisLeft(yScale).ticks(5))
            .style('color', 'var(--text-secondary)');

        // 0 기준선
        g.append('line')
            .attr('x1', 0)
            .attr('x2', innerWidth)
            .attr('y1', yScale(0))
            .attr('y2', yScale(0))
            .attr('stroke', '#666')
            .attr('stroke-dasharray', '4,4');

        // 파형 라인
        const line = d3.line()
            .x(d => xScale(d.index))
            .y(d => yScale(d.value))
            .curve(d3.curveLinear);

        g.append('path')
            .datum(samples)
            .attr('fill', 'none')
            .attr('stroke', '#3498db')
            .attr('stroke-width', 1.5)
            .attr('d', line);

        // 레이블
        g.append('text')
            .attr('x', innerWidth / 2)
            .attr('y', -5)
            .attr('text-anchor', 'middle')
            .attr('fill', 'var(--text-primary)')
            .style('font-size', '14px')
            .style('font-weight', '600')
            .text(`${label} - ${audioData.length} samples`);
    }

    async playCompareHighQuality() {
        if (!this.resultHighQuality) {
            alert('먼저 비교를 실행하세요.');
            return;
        }
        try {
            const wavData = this.float32ToWav(this.resultHighQuality);
            await this.player.playWavData(wavData);
        } catch (error) {
            console.error('재생 실패:', error);
            alert('재생 실패: ' + error.message);
        }
    }

    async playCompareExternal() {
        if (!this.resultExternal) {
            alert('먼저 비교를 실행하세요.');
            return;
        }
        try {
            const wavData = this.float32ToWav(this.resultExternal);
            await this.player.playWavData(wavData);
        } catch (error) {
            console.error('재생 실패:', error);
            alert('재생 실패: ' + error.message);
        }
    }

    downloadReportJSON() {
        if (!this.benchmarkReport) {
            alert('먼저 비교를 실행하세요.');
            return;
        }

        const jsonString = JSON.stringify(this.benchmarkReport, null, 2);
        const blob = new Blob([jsonString], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `benchmark-report-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    downloadReportHTML() {
        if (!this.benchmarkReport) {
            alert('먼저 비교를 실행하세요.');
            return;
        }

        const report = this.benchmarkReport;
        const hq = report.highQuality;
        const ext = report.external;

        // 승자 판정
        const winners = {
            snr: hq.quality.snr > ext.quality.snr ? 'HighQuality' : 'External',
            rmsError: hq.quality.rmsError < ext.quality.rmsError ? 'HighQuality' : 'External',
            peakError: hq.quality.peakError < ext.quality.peakError ? 'HighQuality' : 'External',
            spectralDistortion: hq.quality.spectralDistortion < ext.quality.spectralDistortion ? 'HighQuality' : 'External',
            correlation: hq.quality.correlation > ext.quality.correlation ? 'HighQuality' : 'External',
            speed: hq.processingTime < ext.processingTime ? 'HighQuality' : 'External'
        };

        // 전체 승자 (품질 메트릭 기준)
        let hqWins = 0;
        let extWins = 0;
        Object.keys(winners).forEach(key => {
            if (key !== 'speed') {
                if (winners[key] === 'HighQuality') hqWins++;
                else extWins++;
            }
        });
        const overallWinner = hqWins > extWins ? 'HighQuality' : 'External';

        const html = `
<!DOCTYPE html>
<html lang="ko">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>음성 변조 벤치마크 리포트</title>
    <style>
        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #0a0e27;
            color: #ffffff;
            padding: 40px;
            line-height: 1.6;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: #1a1f3a;
            border: 1px solid #1e293b;
            border-radius: 24px;
            padding: 40px;
        }
        h1 {
            text-align: center;
            background: linear-gradient(135deg, #6366f1, #a855f7);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 40px;
        }
        h2 {
            color: #ffffff;
            margin-top: 32px;
            margin-bottom: 16px;
            border-bottom: 2px solid #1e293b;
            padding-bottom: 8px;
        }
        .info-section {
            background: #12172e;
            padding: 20px;
            border-radius: 12px;
            margin-bottom: 24px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
            background: #12172e;
            border-radius: 12px;
            overflow: hidden;
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #1e293b;
        }
        th {
            background: #1f2543;
            font-weight: 600;
        }
        .winner {
            color: #10b981;
            font-weight: bold;
        }
        .summary {
            background: linear-gradient(135deg, #6366f1, #a855f7);
            padding: 24px;
            border-radius: 12px;
            text-align: center;
            margin: 32px 0;
        }
        .summary h2 {
            color: white;
            border: none;
            margin: 0;
        }
        .timestamp {
            color: #94a3b8;
            font-size: 14px;
            text-align: center;
            margin-bottom: 24px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎵 음성 변조 벤치마크 리포트</h1>
        <div class="timestamp">생성 시각: ${new Date(report.timestamp).toLocaleString('ko-KR')}</div>

        <div class="summary">
            <h2>전체 승자: ${overallWinner === 'HighQuality' ? '🏆 HighQuality (자체 구현)' : '🏆 External (SoundTouch)'}</h2>
            <p style="margin-top: 12px;">HighQuality: ${hqWins}승 | External: ${extWins}승</p>
        </div>

        <h2>📊 기본 정보</h2>
        <div class="info-section">
            <p><strong>샘플레이트:</strong> ${report.sampleRate} Hz</p>
            <p><strong>원본 길이:</strong> ${report.originalLength} samples (${(report.originalLength / report.sampleRate).toFixed(2)}초)</p>
            <p><strong>Pitch 편집:</strong> ${report.edits.pitchEdits.length}개 프레임</p>
            <p><strong>Duration 편집:</strong> ${report.edits.durationRegions.length}개 구간</p>
        </div>

        <h2>⏱️ 처리 시간 비교</h2>
        <table>
            <thead>
                <tr>
                    <th>파이프라인</th>
                    <th>처리 시간</th>
                    <th>출력 길이</th>
                    <th>승자</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>HighQuality (자체 구현)</td>
                    <td>${hq.processingTime.toFixed(2)} ms</td>
                    <td>${hq.outputLength} samples</td>
                    <td>${winners.speed === 'HighQuality' ? '<span class="winner">🏆 빠름</span>' : ''}</td>
                </tr>
                <tr>
                    <td>External (SoundTouch)</td>
                    <td>${ext.processingTime.toFixed(2)} ms</td>
                    <td>${ext.outputLength} samples</td>
                    <td>${winners.speed === 'External' ? '<span class="winner">🏆 빠름</span>' : ''}</td>
                </tr>
                <tr>
                    <td colspan="4" style="text-align: center; background: #1a1f3a;">
                        <strong>속도 차이:</strong> ${(ext.processingTime / hq.processingTime).toFixed(2)}x
                    </td>
                </tr>
            </tbody>
        </table>

        <h2>🎯 품질 메트릭 비교</h2>
        <table>
            <thead>
                <tr>
                    <th>메트릭</th>
                    <th>HighQuality</th>
                    <th>External</th>
                    <th>승자</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td>SNR (Signal-to-Noise Ratio)</td>
                    <td>${hq.quality.snr.toFixed(2)} dB</td>
                    <td>${ext.quality.snr.toFixed(2)} dB</td>
                    <td><span class="winner">🏆 ${winners.snr}</span></td>
                </tr>
                <tr>
                    <td>RMS Error (낮을수록 좋음)</td>
                    <td>${hq.quality.rmsError.toFixed(4)}</td>
                    <td>${ext.quality.rmsError.toFixed(4)}</td>
                    <td><span class="winner">🏆 ${winners.rmsError}</span></td>
                </tr>
                <tr>
                    <td>Peak Error (낮을수록 좋음)</td>
                    <td>${hq.quality.peakError.toFixed(4)}</td>
                    <td>${ext.quality.peakError.toFixed(4)}</td>
                    <td><span class="winner">🏆 ${winners.peakError}</span></td>
                </tr>
                <tr>
                    <td>Spectral Distortion (낮을수록 좋음)</td>
                    <td>${hq.quality.spectralDistortion.toFixed(2)} dB</td>
                    <td>${ext.quality.spectralDistortion.toFixed(2)} dB</td>
                    <td><span class="winner">🏆 ${winners.spectralDistortion}</span></td>
                </tr>
                <tr>
                    <td>Correlation (높을수록 좋음)</td>
                    <td>${hq.quality.correlation.toFixed(4)}</td>
                    <td>${ext.quality.correlation.toFixed(4)}</td>
                    <td><span class="winner">🏆 ${winners.correlation}</span></td>
                </tr>
            </tbody>
        </table>

        <h2>💡 추천 사항</h2>
        <div class="info-section">
            ${overallWinner === 'HighQuality'
                ? '<p><strong>HighQuality (자체 구현)</strong>를 추천합니다. 전반적으로 더 높은 품질 메트릭을 보입니다.</p>'
                : '<p><strong>External (SoundTouch)</strong>를 추천합니다. 전반적으로 더 높은 품질 메트릭을 보입니다.</p>'}
            ${winners.speed !== overallWinner
                ? '<p>단, 처리 속도는 <strong>' + winners.speed + '</strong>가 더 빠릅니다. 실시간 처리가 필요한 경우 고려하세요.</p>'
                : ''}
        </div>
    </div>
</body>
</html>
        `;

        const blob = new Blob([html], { type: 'text/html' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `benchmark-report-${Date.now()}.html`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }

    // 슬라이더 배경 업데이트 (채워진 부분 시각화)
    updateSliderBackground(slider) {
        const min = parseFloat(slider.min) || 0;
        const max = parseFloat(slider.max) || 100;
        const value = parseFloat(slider.value) || 0;

        // 퍼센트 계산
        const percentage = ((value - min) / (max - min)) * 100;

        // 그라디언트 배경 설정
        slider.style.background = `linear-gradient(to right,
            var(--accent-start) 0%,
            var(--accent-end) ${percentage}%,
            var(--bg-secondary) ${percentage}%,
            var(--bg-secondary) 100%)`;
    }

    // 유틸리티 함수
    wavToFloat32(wavData) {
        // ArrayBuffer 또는 TypedArray 둘 다 처리
        let buffer, length;

        if (wavData instanceof ArrayBuffer) {
            buffer = wavData;
            length = wavData.byteLength;
        } else {
            buffer = wavData.buffer;
            length = wavData.length;
        }

        const dataView = new DataView(buffer);
        const samples = [];

        for (let i = 44; i < length; i += 2) {
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

    // ====== 통합 에디터 메서드 ======

    async analyzeUnified() {
        if (!this.originalAudio) {
            alert("먼저 음성을 녹음하거나 업로드하세요.");
            return;
        }

        try {
            // Pitch 분석
            const pitchData = await this.analyzePitchData();

            // Duration 데이터 (기본 비어있음)
            const durationData = [];

            // 통합 그래프 렌더링
            this.unifiedEditor.render(pitchData, durationData);

            // 샘플 생성 버튼 활성화
            document.getElementById("generate-sample").disabled = false;
            document.getElementById("reset-unified").disabled = false;

        } catch (error) {
            console.error("분석 오류:", error);
            alert("음성 분석 중 오류가 발생했습니다: " + error.message);
        }
    }

    async analyzePitchData() {
        const data = this.originalAudio;
        const sampleRate = this.sampleRate;

        console.log('analyzePitchData - data type:', data.constructor.name);
        console.log('analyzePitchData - data length:', data.length);
        console.log('analyzePitchData - sampleRate:', sampleRate);

        // WASM 모듈로 피치 분석
        const dataPtr = this.module._malloc(data.length * 4);
        console.log('analyzePitchData - allocated dataPtr:', dataPtr);

        // Float32Array를 HEAPF32에 복사
        this.module.HEAPF32.set(data, dataPtr / 4);

        const result = this.module.analyzePitch(dataPtr, data.length, sampleRate, 0.02);
        console.log('analyzePitchData - result type:', Array.isArray(result) ? 'Array' : typeof result);
        console.log('analyzePitchData - result length:', result?.length);

        this.module._free(dataPtr);

        if (!result) {
            throw new Error('Pitch 분석 실패: WASM 모듈이 결과를 반환하지 않았습니다.');
        }

        // WASM 함수가 이미 JavaScript 배열을 반환하는 경우
        if (Array.isArray(result)) {
            console.log('analyzePitchData - 이미 파싱된 배열을 받았습니다');
            return result;
        }

        // 포인터를 반환하는 경우 (레거시)
        const resultPtr = result;
        const numPoints = this.module.HEAP32[resultPtr / 4];
        const pitchData = [];

        for (let i = 0; i < numPoints; i++) {
            const offset = resultPtr + 4 + i * 12;
            const time = this.module.HEAPF32[offset / 4];
            const frequency = this.module.HEAPF32[offset / 4 + 1];
            const confidence = this.module.HEAPF32[offset / 4 + 2];

            pitchData.push({ time, frequency, confidence });
        }

        this.module._free(resultPtr);
        return pitchData;
    }

    async generateSample() {
        if (!this.originalAudio) {
            alert("먼저 음성을 분석하세요.");
            return;
        }

        try {
            document.getElementById("sample-status").textContent = "⏳ 샘플 생성 중...";

            // 편집 데이터 가져오기
            const edits = this.unifiedEditor.getEdits();

            // 선택된 알고리즘
            const pitchAlgo = document.getElementById("pitch-algorithm").value;
            const durationAlgo = document.getElementById("duration-algorithm").value;
            const processOrder = document.getElementById("processing-order").value;

            // 샘플 생성 (알고리즘과 순서에 따라)
            this.sampleAudio = await this.processAudioWithEdits(
                this.originalAudio,
                edits,
                pitchAlgo,
                durationAlgo,
                processOrder
            );

            // 재생 버튼 활성화
            document.getElementById("play-sample").disabled = false;
            document.getElementById("stop-sample").disabled = false;
            document.getElementById("download-sample").disabled = false;

            document.getElementById("sample-status").textContent =
                `✅ 샘플 생성 완료 (${(this.sampleAudio.length / this.sampleRate).toFixed(2)}초)`;

        } catch (error) {
            console.error("샘플 생성 오류:", error);
            document.getElementById("sample-status").textContent = "❌ 샘플 생성 실패";
            alert("샘플 생성 중 오류가 발생했습니다: " + error.message);
        }
    }

    async processAudioWithEdits(audio, edits, pitchAlgo, durationAlgo, processOrder) {
        let result = new Float32Array(audio);

        // 새로운 파이프라인 아키텍처 사용
        if (edits.interpolatedFrames && edits.interpolatedFrames.length > 0) {
            try {
                // C++ processAudioWithPipeline 호출
                const dataPtr = this.module._malloc(audio.length * 4);
                this.module.HEAPF32.set(audio, dataPtr / 4);

                // Preview mode 여부 (빠른 생성인지 최종 생성인지)
                const previewMode = pitchAlgo === "psola";

                console.log(`✓ Processing with pipeline: pitch=${pitchAlgo}, duration=${durationAlgo || 'none'}, preview=${previewMode}`);

                const resultView = this.module.processAudioWithPipeline(
                    dataPtr,
                    audio.length,
                    this.sampleRate,
                    edits.interpolatedFrames,
                    pitchAlgo,
                    durationAlgo || 'none',  // duration algorithm
                    previewMode,
                    3.0,   // gradientThreshold
                    0.02   // frameInterval
                );

                // 결과를 Float32Array로 복사
                result = convertPipelineResultToFloat32Array(resultView);

                // 메모리 해제
                this.module._free(dataPtr);

                console.log(`✓ Pipeline processing complete: ${result.length} samples`);
            } catch (error) {
                console.error('Pipeline processing failed:', error);
                throw error;  // 에러를 상위로 전달
            }
        } else {
            console.warn('⚠️ No interpolated frames available. Please use the unified editor for variable pitch/duration.');
        }

        return result;
    }

    // applyInterpolatedPitchShift 제거됨 - 새 Pipeline 아키텍처 사용
    // convertPipelineResultToFloat32Array는 audio-utils.js로 이동됨

    /**
     * 특정 시간의 보간된 semitones 계산
     */
    getInterpolatedSemitones(time, editPoints) {
        if (editPoints.length === 0) {
            return 0;
        }

        // 현재 시간 이전과 이후의 편집 포인트 찾기
        let beforeEdit = null;
        let afterEdit = null;

        for (let i = 0; i < editPoints.length; i++) {
            if (editPoints[i].time <= time) {
                beforeEdit = editPoints[i];
            }
            if (editPoints[i].time >= time && !afterEdit) {
                afterEdit = editPoints[i];
                break;
            }
        }

        if (!beforeEdit && !afterEdit) {
            // 편집 포인트 없음
            return 0;
        } else if (!beforeEdit) {
            // 첫 번째 편집 포인트 이전 - 원본 유지
            return 0;
        } else if (!afterEdit) {
            // 마지막 편집 포인트 이후 - 원본 유지
            return 0;
        } else if (beforeEdit.time === afterEdit.time) {
            // 정확히 편집 포인트 위치
            return beforeEdit.semitones;
        } else {
            // 두 편집 포인트 사이 - 선형 보간
            const t = (time - beforeEdit.time) / (afterEdit.time - beforeEdit.time);
            return beforeEdit.semitones + t * (afterEdit.semitones - beforeEdit.semitones);
        }
    }

    // applyDurationEdits, applyPitchShiftWithAlgorithm, applyTimeStretchWithAlgorithm 제거됨
    // 모두 새 Pipeline 아키텍처로 대체됨

    async playSample() {
        if (this.sampleAudio) {
            await this.samplePlayer.playFloat32Array(this.sampleAudio, this.sampleRate);
        }
    }

    stopSample() {
        this.samplePlayer.stop();
    }

    downloadSample() {
        if (!this.sampleAudio) return;

        const wavData = this.float32ToWav(this.sampleAudio);
        const blob = new Blob([wavData], { type: "audio/wav" });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = "sample_" + new Date().getTime() + ".wav";
        a.click();
        URL.revokeObjectURL(url);
    }

    resetUnified() {
        if (this.unifiedEditor) {
            this.unifiedEditor.reset();
        }
        this.sampleAudio = null;
        document.getElementById("generate-sample").disabled = false;
        document.getElementById("play-sample").disabled = true;
        document.getElementById("stop-sample").disabled = true;
        document.getElementById("download-sample").disabled = true;
        document.getElementById("sample-status").textContent = "";
    }

    updateProcessingDescription() {
        const order = document.getElementById("processing-order").value;
        const descriptions = {
            "pitch-first": "<strong>Pitch → Duration:</strong> 음높이를 먼저 변경한 후 재생 속도를 조절합니다.",
            "duration-first": "<strong>Duration → Pitch:</strong> 재생 속도를 먼저 조절한 후 음높이를 변경합니다.",
            "direct": "<strong>Direct (통합 처리):</strong> 한 번에 모든 변환을 적용합니다 (가장 빠름)."
        };
        document.getElementById("processing-description").innerHTML =
            `💡 ${descriptions[order]}`;
    }
}
