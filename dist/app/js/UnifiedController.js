import { PerformanceReport } from './PerformanceReport.js';

// JavaScript engine imports (상대 경로로 ../js/js 폴더 참조)
import { AudioRecorder } from '../../js/js/audio/AudioRecorder.js';
import { AudioPlayer } from '../../js/js/audio/AudioPlayer.js';
import { AudioBuffer as JSAudioBuffer } from '../../js/js/audio/AudioBuffer.js';
import { VoiceFilter } from '../../js/js/effects/VoiceFilter.js';
import { AudioReverser } from '../../js/js/effects/AudioReverser.js';
import { PitchAnalyzer } from '../../js/js/analysis/PitchAnalyzer.js';
import { SimplePitchShifter } from '../../js/js/dsp/SimplePitchShifter.js';
import { SimpleTimeStretcher } from '../../js/js/dsp/SimpleTimeStretcher.js';
import { PerformanceChecker as JSPerformanceChecker } from '../../js/js/performance/PerformanceChecker.js';

/**
 * UnifiedController - C++/JS 엔진 통합 컨트롤러
 * 엔진 토글, 성능 측정, 보고서 생성
 */
class UnifiedController {
    constructor() {
        this.currentEngine = 'js'; // 'js' or 'cpp'
        this.wasmReady = false;

        // JavaScript 엔진 컴포넌트
        this.jsRecorder = new AudioRecorder();
        this.jsPlayer = new AudioPlayer();
        this.jsVoiceFilter = new VoiceFilter();
        this.jsReverser = new AudioReverser();
        this.jsPitchAnalyzer = new PitchAnalyzer();
        this.jsPitchShifter = new SimplePitchShifter();
        this.jsTimeStretcher = new SimpleTimeStretcher();
        this.jsPerformanceChecker = new JSPerformanceChecker();

        // C++ 엔진 (WASM Module)
        this.cppModule = null;
        this.cppAudioBuffer = null;

        // 공통 데이터
        this.originalAudio = null;
        this.processedAudio = null;
        this.pitchData = [];

        // 성능 보고서
        this.performanceReport = new PerformanceReport();

        this.init();
    }

    async init() {
        this.setupEventListeners();
        this.initWasm();
        this.performanceReport.loadFromLocalStorage();
        this.performanceReport.renderReportList(); // 보고서 목록 초기 렌더링
        console.log('UnifiedController initialized');
    }

    /**
     * WASM 초기화
     */
    async initWasm() {
        try {
            console.log('Initializing WASM...');
            this.cppModule = await Module({
                locateFile(path) {
                    if (path.endsWith('.wasm')) {
                        return '../cpp/main.wasm';
                    }
                    return path;
                },
                onRuntimeInitialized: () => {
                    console.log('WASM Runtime initialized!');
                    this.wasmReady = true;
                }
            });
            window.Module = this.cppModule;
        } catch (error) {
            console.error('WASM initialization error:', error);
            alert('C++ 엔진 로드 실패. JavaScript 모드만 사용 가능합니다.');
        }
    }

    setupEventListeners() {
        // Recording
        document.getElementById('startRecord').addEventListener('click', () => this.startRecording());
        document.getElementById('stopRecord').addEventListener('click', () => this.stopRecording());

        // File upload
        document.getElementById('uploadFile').addEventListener('click', () => {
            document.getElementById('fileInput').click();
        });
        document.getElementById('fileInput').addEventListener('change', (e) => this.handleFileUpload(e));

        // Playback
        document.getElementById('playOriginal').addEventListener('click', () => this.playOriginal());
        document.getElementById('playProcessed').addEventListener('click', () => this.playProcessed());
        document.getElementById('stopProcessed').addEventListener('click', () => this.stopProcessed());

        // Effects
        document.getElementById('applyAllEffects').addEventListener('click', () => this.applyEffects());
        document.getElementById('resetEffects').addEventListener('click', () => this.resetEffects());
        document.getElementById('downloadProcessed').addEventListener('click', () => this.downloadProcessed());

        // Filter params
        document.getElementById('filterType').addEventListener('change', () => this.updateFilterParams());

        // Value displays
        document.getElementById('pitchShift').addEventListener('input', (e) => {
            document.getElementById('pitchValue').textContent = e.target.value;
        });
        document.getElementById('timeStretch').addEventListener('input', (e) => {
            document.getElementById('timeValue').textContent = e.target.value + 'x';
        });

        // Engine toggle
        document.getElementById('engineToggle').addEventListener('change', (e) => {
            this.toggleEngine(e.target.checked);
        });

        // Report panel
        document.getElementById('toggleReport').addEventListener('click', () => this.toggleReportPanel());
        document.getElementById('closeReport').addEventListener('click', () => this.toggleReportPanel());

        // Report tabs
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => this.switchTab(e.target.dataset.tab));
        });

        // Report actions
        document.getElementById('exportJSON').addEventListener('click', () => {
            this.performanceReport.exportJSON();
        });
        document.getElementById('exportCSV').addEventListener('click', () => {
            this.performanceReport.exportCSV();
        });
        document.getElementById('clearReports').addEventListener('click', () => {
            this.performanceReport.clearAll();
        });
    }

    /**
     * 엔진 토글 (JS <-> C++)
     */
    toggleEngine(useCpp) {
        this.currentEngine = useCpp ? 'cpp' : 'js';
        const label = document.getElementById('engineLabel');
        label.textContent = useCpp ? 'C++' : 'JavaScript';

        this.setStatus(`엔진 변경: ${label.textContent}`);
        console.log(`Engine switched to: ${this.currentEngine}`);

        if (useCpp && !this.wasmReady) {
            alert('C++ 엔진이 아직 로드되지 않았습니다. 잠시 후 다시 시도하세요.');
            document.getElementById('engineToggle').checked = false;
            this.currentEngine = 'js';
            label.textContent = 'JavaScript';
        }
    }

    /**
     * 보고서 패널 토글
     */
    toggleReportPanel() {
        const panel = document.getElementById('reportPanel');
        panel.classList.toggle('hidden');

        if (!panel.classList.contains('hidden')) {
            // 패널이 열릴 때 보고서 렌더링
            this.performanceReport.renderComparison();
            this.performanceReport.renderCppDetails();
            this.performanceReport.renderJsDetails();
        }
    }

    /**
     * 보고서 탭 전환
     */
    switchTab(tabName) {
        // 탭 버튼 활성화
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.tab === tabName);
        });

        // 탭 콘텐츠 표시
        document.querySelectorAll('.tab-content').forEach(content => {
            content.classList.toggle('active', content.id === `${tabName}Tab`);
        });

        // 해당 탭의 보고서 렌더링
        if (tabName === 'comparison') {
            this.performanceReport.renderComparison();
        } else if (tabName === 'cpp') {
            this.performanceReport.renderCppDetails();
        } else if (tabName === 'js') {
            this.performanceReport.renderJsDetails();
        }
    }

    /**
     * 녹음 시작
     */
    async startRecording() {
        try {
            await this.jsRecorder.start();
            document.getElementById('startRecord').disabled = true;
            document.getElementById('stopRecord').disabled = false;
            this.setStatus('녹음 중...');
        } catch (error) {
            console.error('Recording error:', error);
            alert('녹음을 시작할 수 없습니다.');
        }
    }

    /**
     * 녹음 중지
     */
    async stopRecording() {
        try {
            this.originalAudio = await this.jsRecorder.stop();
            document.getElementById('startRecord').disabled = false;
            document.getElementById('stopRecord').disabled = true;
            document.getElementById('playOriginal').disabled = false;
            document.getElementById('applyAllEffects').disabled = false;

            this.setStatus('녹음 완료');
            await this.analyzePitch();
        } catch (error) {
            console.error('Stop recording error:', error);
        }
    }

    /**
     * 파일 업로드
     */
    async handleFileUpload(event) {
        const file = event.target.files[0];
        if (!file) return;

        this.setStatus('파일 로딩 중...');

        try {
            const arrayBuffer = await file.arrayBuffer();
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const audioBufferData = await audioContext.decodeAudioData(arrayBuffer);

            const sampleRate = audioBufferData.sampleRate;
            const channelData = audioBufferData.getChannelData(0);

            this.originalAudio = new JSAudioBuffer(sampleRate, 1);
            this.originalAudio.setData(channelData);

            document.getElementById('fileInfo').textContent = file.name;
            document.getElementById('playOriginal').disabled = false;
            document.getElementById('applyAllEffects').disabled = false;

            this.setStatus('파일 로드 완료');
            await this.analyzePitch();
        } catch (error) {
            console.error('File upload error:', error);
            alert('파일을 로드할 수 없습니다.');
        }
    }

    /**
     * 피치 분석
     */
    async analyzePitch() {
        if (!this.originalAudio) return;

        this.setStatus('피치 분석 중...');

        try {
            // JS 엔진 사용 (C++/JS 모두 동일한 분석 결과 사용)
            this.pitchData = this.jsPitchAnalyzer.analyze(this.originalAudio);
            this.visualizePitch();
            this.setStatus('분석 완료');
        } catch (error) {
            console.error('Pitch analysis error:', error);
            this.setStatus('분석 실패');
        }
    }

    /**
     * 피치 시각화
     */
    visualizePitch() {
        const chart = document.getElementById('pitch-chart');
        chart.innerHTML = '';

        if (this.pitchData.length === 0) {
            chart.innerHTML = '<div class="empty-state"><p>피치 데이터 없음</p></div>';
            return;
        }

        const margin = { top: 20, right: 30, bottom: 40, left: 50 };
        const width = chart.clientWidth - margin.left - margin.right;
        const height = chart.clientHeight - margin.top - margin.bottom;

        const svg = d3.select('#pitch-chart')
            .append('svg')
            .attr('width', width + margin.left + margin.right)
            .attr('height', height + margin.top + margin.bottom)
            .append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        // Scales
        const x = d3.scaleLinear()
            .domain(d3.extent(this.pitchData, d => d.time))
            .range([0, width]);

        const y = d3.scaleLinear()
            .domain([
                d3.min(this.pitchData, d => d.frequency) * 0.9,
                d3.max(this.pitchData, d => d.frequency) * 1.1
            ])
            .range([height, 0]);

        // Line
        const line = d3.line()
            .x(d => x(d.time))
            .y(d => y(d.frequency))
            .curve(d3.curveMonotoneX);

        svg.append('path')
            .datum(this.pitchData)
            .attr('fill', 'none')
            .attr('stroke', 'var(--accent-color)')
            .attr('stroke-width', 2)
            .attr('d', line);

        // X axis
        svg.append('g')
            .attr('class', 'axis x-axis')
            .attr('transform', `translate(0,${height})`)
            .call(d3.axisBottom(x)
                .ticks(10)
                .tickFormat(d => `${d.toFixed(1)}s`)
            );

        svg.append('text')
            .attr('class', 'axis-label')
            .attr('x', width / 2)
            .attr('y', height + 35)
            .attr('text-anchor', 'middle')
            .text('Time (seconds)');

        // Y axis
        svg.append('g')
            .attr('class', 'axis y-axis')
            .call(d3.axisLeft(y)
                .ticks(8)
                .tickFormat(d => `${d.toFixed(0)} Hz`)
            );

        svg.append('text')
            .attr('class', 'axis-label')
            .attr('transform', 'rotate(-90)')
            .attr('x', -height / 2)
            .attr('y', -40)
            .attr('text-anchor', 'middle')
            .text('Pitch (Hz)');
    }

    /**
     * 효과 적용 (JS와 C++ 둘 다 순차 실행)
     */
    async applyEffects() {
        if (!this.originalAudio) return;

        this.setStatus('효과 적용 중 (JS → C++)...');

        try {
            // 현재 설정 저장
            const settings = {
                pitch: parseFloat(document.getElementById('pitchShift').value),
                timeStretch: parseFloat(document.getElementById('timeStretch').value),
                filter: document.getElementById('filterType').value,
                filterParam1: parseFloat(document.getElementById('filterParam1').value),
                filterParam2: parseFloat(document.getElementById('filterParam2').value),
                reverse: document.getElementById('reversePlayback').checked
            };

            // 원래 엔진 저장
            const originalEngine = this.currentEngine;

            // 1. JavaScript 엔진으로 처리
            this.setStatus('JavaScript 엔진 처리 중...');
            await this.applyEffectsJS();
            const jsReport = this.performanceReport.jsReport;

            // 2. C++ 엔진으로 처리
            this.setStatus('C++ 엔진 처리 중...');
            await this.applyEffectsCpp();
            const cppReport = this.performanceReport.cppReport;

            // 3. 세션 보고서 저장
            this.performanceReport.saveSessionReport(cppReport, jsReport, settings);

            // 4. 원래 엔진으로 복원 (마지막 처리 결과 유지)
            this.currentEngine = originalEngine;

            // UI 업데이트
            document.getElementById('playProcessed').disabled = false;
            document.getElementById('stopProcessed').disabled = false;
            document.getElementById('downloadProcessed').disabled = false;

            // 보고서 목록 업데이트
            this.performanceReport.renderReportList();

            this.setStatus('효과 적용 완료 (JS + C++)');
        } catch (error) {
            console.error('Apply effects error:', error);
            this.setStatus('효과 적용 실패: ' + error.message);
        }
    }

    /**
     * JavaScript 엔진으로 효과 적용
     */
    async applyEffectsJS() {
        this.jsPerformanceChecker.reset();

        let audioData = this.originalAudio.getData();
        const sampleRate = this.originalAudio.getSampleRate();

        // Pitch Shift
        const pitchShift = parseFloat(document.getElementById('pitchShift').value);
        if (Math.abs(pitchShift) > 0.01) {
            this.setStatus('피치 변경 중 (JS)...');
            this.jsPerformanceChecker.start('pitch_total');
            audioData = this.jsPitchShifter.process(audioData, sampleRate, pitchShift, this.jsPerformanceChecker);
            this.jsPerformanceChecker.end('pitch_total');
        }

        // Time Stretch
        const timeStretch = parseFloat(document.getElementById('timeStretch').value);
        if (Math.abs(timeStretch - 1.0) > 0.01) {
            this.setStatus('속도 조절 중 (JS)...');
            this.jsPerformanceChecker.start('duration_total');
            audioData = this.jsTimeStretcher.process(audioData, sampleRate, timeStretch, this.jsPerformanceChecker);
            this.jsPerformanceChecker.end('duration_total');
        }

        // AudioBuffer로 변환
        let audio = new JSAudioBuffer(sampleRate, 1);
        audio.setData(audioData);

        // Filter
        const filterType = parseInt(document.getElementById('filterType').value);
        if (!isNaN(filterType)) {
            this.setStatus('필터 적용 중 (JS)...');
            const param1 = parseFloat(document.getElementById('filterParam1').value);
            const param2 = parseFloat(document.getElementById('filterParam2').value);
            this.jsPerformanceChecker.start('filter_total');
            audio = this.jsVoiceFilter.applyFilter(audio, filterType, param1, param2);
            this.jsPerformanceChecker.end('filter_total');
        }

        // Reverse
        if (document.getElementById('reversePlayback').checked) {
            this.setStatus('역재생 처리 중 (JS)...');
            this.jsPerformanceChecker.start('reverse_total');
            audio = this.jsReverser.reverse(audio);
            this.jsPerformanceChecker.end('reverse_total');
        }

        this.processedAudio = audio;

        // 성능 보고서 저장
        const report = this.jsPerformanceChecker.getReportJSON();
        this.performanceReport.setJsReport(report);

        console.log('JS Performance Report:', report);
        console.log('JS Performance Report (JSON):', JSON.stringify(report, null, 2));
    }

    /**
     * C++ 엔진으로 효과 적용
     */
    async applyEffectsCpp() {
        if (!this.wasmReady) {
            throw new Error('WASM not ready');
        }

        // C++ PerformanceChecker 생성
        const cppPerfChecker = new this.cppModule.PerformanceChecker();

        let audioData = this.originalAudio.getData();
        const sampleRate = this.originalAudio.getSampleRate();

        // Pitch Shift
        const pitchShift = parseFloat(document.getElementById('pitchShift').value);
        if (Math.abs(pitchShift) > 0.01) {
            this.setStatus('피치 변경 중 (C++)...');
            cppPerfChecker.startFeature('pitch_total');

            // Float32Array를 WASM 메모리에 복사
            const numBytes = audioData.length * audioData.BYTES_PER_ELEMENT;
            const dataPtr = this.cppModule._malloc(numBytes);
            this.cppModule.HEAPF32.set(audioData, dataPtr >> 2);

            // 처리 (PerformanceChecker 객체를 직접 전달)
            const resultArray = this.cppModule.applyUniformPitchShift(
                dataPtr,
                audioData.length,
                sampleRate,
                pitchShift,
                'simple',
                cppPerfChecker
            );

            // 결과 복사
            audioData = new Float32Array(resultArray.length);
            for (let i = 0; i < resultArray.length; i++) {
                audioData[i] = resultArray[i];
            }

            this.cppModule._free(dataPtr);
            cppPerfChecker.endFeature();
        }

        // Time Stretch
        const timeStretch = parseFloat(document.getElementById('timeStretch').value);
        if (Math.abs(timeStretch - 1.0) > 0.01) {
            this.setStatus('속도 조절 중 (C++)...');
            cppPerfChecker.startFeature('duration_total');

            // Float32Array를 WASM 메모리에 복사
            const numBytes = audioData.length * audioData.BYTES_PER_ELEMENT;
            const dataPtr = this.cppModule._malloc(numBytes);
            this.cppModule.HEAPF32.set(audioData, dataPtr >> 2);

            // 처리 (PerformanceChecker 객체를 직접 전달)
            const resultArray = this.cppModule.applyUniformTimeStretch(
                dataPtr,
                audioData.length,
                sampleRate,
                timeStretch,
                'simple',
                cppPerfChecker
            );

            // 결과 복사
            audioData = new Float32Array(resultArray.length);
            for (let i = 0; i < resultArray.length; i++) {
                audioData[i] = resultArray[i];
            }

            this.cppModule._free(dataPtr);
            cppPerfChecker.endFeature();
        }

        // AudioBuffer로 변환
        let audio = new JSAudioBuffer(sampleRate, 1);
        audio.setData(audioData);

        // Filter
        const filterType = parseInt(document.getElementById('filterType').value);
        if (!isNaN(filterType)) {
            this.setStatus('필터 적용 중 (C++)...');
            cppPerfChecker.startFeature('filter_total');

            const param1 = parseFloat(document.getElementById('filterParam1').value);
            const param2 = parseFloat(document.getElementById('filterParam2').value);

            // Float32Array를 WASM 메모리에 복사
            const data = audio.getData();
            const numBytes = data.length * data.BYTES_PER_ELEMENT;
            const dataPtr = this.cppModule._malloc(numBytes);
            this.cppModule.HEAPF32.set(data, dataPtr >> 2);

            // 처리
            const resultArray = this.cppModule.applyVoiceFilter(
                dataPtr,
                data.length,
                sampleRate,
                filterType,
                param1,
                param2
            );

            // 결과 복사
            const outputData = new Float32Array(resultArray.length);
            for (let i = 0; i < resultArray.length; i++) {
                outputData[i] = resultArray[i];
            }

            audio = new JSAudioBuffer(sampleRate, 1);
            audio.setData(outputData);

            this.cppModule._free(dataPtr);
            cppPerfChecker.endFeature();
        }

        // Reverse
        if (document.getElementById('reversePlayback').checked) {
            this.setStatus('역재생 처리 중 (C++)...');
            cppPerfChecker.startFeature('reverse_total');

            const data = audio.getData();
            const numBytes = data.length * data.BYTES_PER_ELEMENT;
            const dataPtr = this.cppModule._malloc(numBytes);
            this.cppModule.HEAPF32.set(data, dataPtr >> 2);

            // 처리
            const resultArray = this.cppModule.reverseAudio(
                dataPtr,
                data.length,
                sampleRate
            );

            // 결과 복사
            const outputData = new Float32Array(resultArray.length);
            for (let i = 0; i < resultArray.length; i++) {
                outputData[i] = resultArray[i];
            }

            audio = new JSAudioBuffer(sampleRate, 1);
            audio.setData(outputData);

            this.cppModule._free(dataPtr);
            cppPerfChecker.endFeature();
        }

        this.processedAudio = audio;

        // C++ 성능 보고서 가져오기
        const reportJSON = cppPerfChecker.getReportJSON();
        const report = JSON.parse(reportJSON);
        this.performanceReport.setCppReport(report);

        // PerformanceChecker 정리
        cppPerfChecker.delete();

        console.log('C++ Performance Report:', report);
        console.log('C++ Performance Report (JSON):', JSON.stringify(report, null, 2));
    }

    resetEffects() {
        document.getElementById('pitchShift').value = 0;
        document.getElementById('pitchValue').textContent = '0';
        document.getElementById('timeStretch').value = 1.0;
        document.getElementById('timeValue').textContent = '1.0x';
        document.getElementById('filterType').value = 'none';
        document.getElementById('reversePlayback').checked = false;
        this.updateFilterParams();
        this.setStatus('초기화 완료');
    }

    updateFilterParams() {
        const filterType = document.getElementById('filterType').value;
        const paramsDiv = document.getElementById('filterParams');

        if (filterType === 'none') {
            paramsDiv.classList.add('hidden');
        } else {
            paramsDiv.classList.remove('hidden');
        }
    }

    playOriginal() {
        if (this.originalAudio) {
            this.jsPlayer.play(this.originalAudio);
        }
    }

    playProcessed() {
        if (this.processedAudio) {
            this.jsPlayer.play(this.processedAudio);
        }
    }

    stopProcessed() {
        this.jsPlayer.stop();
    }

    downloadProcessed() {
        if (!this.processedAudio) return;

        // TODO: WAV 변환 및 다운로드 구현
        alert('다운로드 기능은 준비 중입니다.');
    }

    setStatus(text) {
        document.getElementById('statusText').textContent = text;
    }
}

// Initialize app
const app = new UnifiedController();
window.app = app;
