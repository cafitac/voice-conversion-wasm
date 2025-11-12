import { AudioRecorder } from './audio-recorder.js';
import { AudioPlayer } from './audio-player.js';
import { InteractiveEditor } from './interactive-editor.js';

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

        // 전역 등록 (region 삭제용)
        window.editor_chart_hq = this.editorHighQuality;
        window.editor_chart_ext = this.editorExternal;

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

        // 탭 전환
        document.getElementById('tab-highquality').addEventListener('click', () => this.switchTab('highquality'));
        document.getElementById('tab-external').addEventListener('click', () => this.switchTab('external'));
        document.getElementById('tab-compare').addEventListener('click', () => this.switchTab('compare'));

        // HighQuality 파이프라인 버튼
        document.getElementById('analyze-hq').addEventListener('click', () => this.analyzeHighQuality());
        document.getElementById('edit-fullscreen-hq').addEventListener('click', () => this.openFullscreenEditor('highquality'));
        document.getElementById('apply-hq').addEventListener('click', () => this.applyEditsHighQuality());
        document.getElementById('reset-hq').addEventListener('click', () => this.resetHighQuality());

        // External 파이프라인 버튼
        document.getElementById('analyze-ext').addEventListener('click', () => this.analyzeExternal());
        document.getElementById('edit-fullscreen-ext').addEventListener('click', () => this.openFullscreenEditor('external'));
        document.getElementById('apply-ext').addEventListener('click', () => this.applyEditsExternal());
        document.getElementById('reset-ext').addEventListener('click', () => this.resetExternal());

        // 비교 버튼
        document.getElementById('compare-run').addEventListener('click', () => this.runComparison());
        document.getElementById('play-compare-hq').addEventListener('click', () => this.playCompareHighQuality());
        document.getElementById('play-compare-ext').addEventListener('click', () => this.playCompareExternal());

        // 리포트 다운로드 버튼
        document.getElementById('download-report-json').addEventListener('click', () => this.downloadReportJSON());
        document.getElementById('download-report-html').addEventListener('click', () => this.downloadReportHTML());

        // 효과 버튼
        document.getElementById('applyPitchShift').addEventListener('click', () => this.applyPitchShift());
        document.getElementById('applyTimeStretch').addEventListener('click', () => this.applyTimeStretch());
        document.getElementById('applyFilter').addEventListener('click', () => this.applyFilter());

        // 재생 및 다운로드
        document.getElementById('playProcessed').addEventListener('click', () => this.playProcessed());
        document.getElementById('downloadProcessed').addEventListener('click', () => this.downloadProcessed());
        document.getElementById('reset').addEventListener('click', () => this.reset());

        // 슬라이더 값 업데이트
        const pitchSlider = document.getElementById('pitchShift');
        pitchSlider.addEventListener('input', (e) => {
            document.getElementById('pitchValue').textContent = e.target.value;
            this.updateSliderBackground(e.target);
        });
        this.updateSliderBackground(pitchSlider);

        const timeSlider = document.getElementById('timeStretch');
        timeSlider.addEventListener('input', (e) => {
            document.getElementById('timeValue').textContent = e.target.value;
            this.updateSliderBackground(e.target);
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
        });
        this.updateSliderBackground(filterParam1);

        const filterParam2 = document.getElementById('filterParam2');
        filterParam2.addEventListener('input', (e) => {
            document.getElementById('param2Value').textContent = e.target.value;
            this.updateSliderBackground(e.target);
        });
        this.updateSliderBackground(filterParam2);
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
        document.getElementById('applyPitchShift').disabled = false;
        document.getElementById('applyTimeStretch').disabled = false;
        document.getElementById('applyFilter').disabled = false;

        // Interactive editor analyze 버튼 활성화
        document.getElementById('analyze-hq').disabled = false;
        document.getElementById('analyze-ext').disabled = false;
        document.getElementById('compare-run').disabled = false;

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
            document.getElementById('applyPitchShift').disabled = false;
            document.getElementById('applyTimeStretch').disabled = false;
            document.getElementById('applyFilter').disabled = false;

            // Interactive editor analyze 버튼 활성화
            document.getElementById('analyze-hq').disabled = false;
            document.getElementById('analyze-ext').disabled = false;
            document.getElementById('compare-run').disabled = false;

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

    setTimeStretchQuality(quality) {
        try {
            this.module.setTimeStretchQuality(quality);
            const currentQualityName = this.module.getTimeStretchQuality();
            document.getElementById('currentTimeStretchQuality').textContent = `현재: ${currentQualityName}`;
            console.log(`TimeStretch quality set to: ${quality} (${currentQualityName})`);
        } catch (error) {
            console.error('Failed to set timestretch quality:', error);
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
        const speed = parseFloat(document.getElementById('timeStretch').value);
        // Speed를 Duration Ratio로 변환
        // speed = 0.5 (느리게) → ratio = 2.0 (duration 2배)
        // speed = 2.0 (빠르게) → ratio = 0.5 (duration 0.5배)
        const ratio = 1.0 / speed;

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
                sampleKeyPoints: keyPointsArray.slice(0, 5).map(kp => ({frame: kp.frameIndex, shift: kp.semitones.toFixed(2)}))
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
                    document.getElementById('applyPitchShift').disabled = false;
                    document.getElementById('applyTimeStretch').disabled = false;
                    document.getElementById('applyFilter').disabled = false;

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
}
