import { AudioRecorder } from '../audio/AudioRecorder.js';
import { AudioPlayer } from '../audio/AudioPlayer.js';
import { AudioBuffer } from '../audio/AudioBuffer.js';
import { VoiceFilter, FilterType } from '../effects/VoiceFilter.js';
import { AudioReverser } from '../effects/AudioReverser.js';
import { PitchAnalyzer } from '../analysis/PitchAnalyzer.js';
import { SimplePitchShifter } from '../dsp/SimplePitchShifter.js';
import { SimpleTimeStretcher } from '../dsp/SimpleTimeStretcher.js';

/**
 * UIController - 메인 앱 컨트롤러 (JavaScript 버전)
 */
class UIController {
    constructor() {
        this.recorder = new AudioRecorder();
        this.player = new AudioPlayer();
        this.voiceFilter = new VoiceFilter();
        this.reverser = new AudioReverser();
        this.pitchAnalyzer = new PitchAnalyzer();
        this.pitchShifter = new SimplePitchShifter();
        this.timeStretcher = new SimpleTimeStretcher();

        this.originalAudio = null;
        this.processedAudio = null;
        this.pitchData = [];

        this.init();
    }

    init() {
        this.setupEventListeners();
        console.log('UIController initialized (JavaScript version)');
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
    }

    async startRecording() {
        try {
            await this.recorder.start();
            document.getElementById('startRecord').disabled = true;
            document.getElementById('stopRecord').disabled = false;
            this.setStatus('녹음 중...');
        } catch (error) {
            console.error('Recording error:', error);
            alert('녹음을 시작할 수 없습니다.');
        }
    }

    async stopRecording() {
        try {
            this.originalAudio = await this.recorder.stop();
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

            this.originalAudio = new AudioBuffer(sampleRate, 1);
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

    async analyzePitch() {
        if (!this.originalAudio) return;

        this.setStatus('피치 분석 중...');

        try {
            this.pitchData = this.pitchAnalyzer.analyze(this.originalAudio);
            this.visualizePitch();
            this.setStatus('분석 완료');
        } catch (error) {
            console.error('Pitch analysis error:', error);
            this.setStatus('분석 실패');
        }
    }

    visualizePitch() {
        const chart = document.getElementById('pitch-chart');
        chart.innerHTML = '';

        if (this.pitchData.length === 0) {
            chart.innerHTML = '<div class="empty-state"><p>피치 데이터 없음</p></div>';
            return;
        }

        // D3.js visualization (C++ 버전과 동일한 스타일)
        const margin = { top: 20, right: 30, bottom: 40, left: 50 };
        const width = chart.clientWidth - margin.left - margin.right;
        const height = chart.clientHeight - margin.top - margin.bottom;

        const svg = d3.select('#pitch-chart')
            .append('svg')
            .attr('width', width + margin.left + margin.right)
            .attr('height', height + margin.top + margin.bottom)
            .append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        // Scales (C++ 버전과 동일: extent 사용 및 y축 범위 조정)
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
            .attr('stroke', '#667eea')
            .attr('stroke-width', 2)
            .attr('d', line);

        // X axis (C++ 버전과 동일: 포맷팅 추가)
        svg.append('g')
            .attr('class', 'axis x-axis')
            .attr('transform', `translate(0,${height})`)
            .call(d3.axisBottom(x)
                .ticks(10)
                .tickFormat(d => `${d.toFixed(1)}s`)
            );

        // X axis label (C++ 버전과 동일)
        svg.append('text')
            .attr('class', 'axis-label')
            .attr('x', width / 2)
            .attr('y', height + 35)
            .attr('text-anchor', 'middle')
            .text('Time (seconds)');

        // Y axis (C++ 버전과 동일: 포맷팅 추가)
        svg.append('g')
            .attr('class', 'axis y-axis')
            .call(d3.axisLeft(y)
                .ticks(8)
                .tickFormat(d => `${d.toFixed(0)} Hz`)
            );

        // Y axis label (C++ 버전과 동일)
        svg.append('text')
            .attr('class', 'axis-label')
            .attr('transform', 'rotate(-90)')
            .attr('x', -height / 2)
            .attr('y', -40)
            .attr('text-anchor', 'middle')
            .text('Pitch (Hz)');
    }

    async applyEffects() {
        if (!this.originalAudio) return;

        this.setStatus('효과 적용 중...');

        try {
            let audioData = this.originalAudio.getData();
            const sampleRate = this.originalAudio.getSampleRate();

            // Pitch Shift
            const pitchShift = parseFloat(document.getElementById('pitchShift').value);
            if (Math.abs(pitchShift) > 0.01) {
                this.setStatus('피치 변경 중...');
                audioData = this.pitchShifter.process(audioData, sampleRate, pitchShift);
            }

            // Time Stretch
            const timeStretch = parseFloat(document.getElementById('timeStretch').value);
            if (Math.abs(timeStretch - 1.0) > 0.01) {
                this.setStatus('속도 조절 중...');
                audioData = this.timeStretcher.process(audioData, sampleRate, timeStretch);
            }

            // AudioBuffer로 변환
            let audio = new AudioBuffer(sampleRate, 1);
            audio.setData(audioData);

            // Filter
            const filterType = parseInt(document.getElementById('filterType').value);
            if (!isNaN(filterType)) {
                this.setStatus('필터 적용 중...');
                const param1 = parseFloat(document.getElementById('filterParam1').value);
                const param2 = parseFloat(document.getElementById('filterParam2').value);
                audio = this.voiceFilter.applyFilter(audio, filterType, param1, param2);
            }

            // Reverse
            if (document.getElementById('reversePlayback').checked) {
                this.setStatus('역재생 처리 중...');
                audio = this.reverser.reverse(audio);
            }

            this.processedAudio = audio;

            document.getElementById('playProcessed').disabled = false;
            document.getElementById('stopProcessed').disabled = false;
            document.getElementById('downloadProcessed').disabled = false;

            this.setStatus('효과 적용 완료');

            // 변환된 오디오의 피치 분석 및 그래프 업데이트 (C++ 버전과 동일)
            await this.analyzeProcessedPitch();
        } catch (error) {
            console.error('Apply effects error:', error);
            this.setStatus('효과 적용 실패: ' + error.message);
        }
    }

    async analyzeProcessedPitch() {
        if (!this.processedAudio) return;

        this.setStatus('변환된 오디오 분석 중...');

        try {
            this.pitchData = this.pitchAnalyzer.analyze(this.processedAudio);
            this.visualizePitch();
            this.setStatus('분석 완료');
        } catch (error) {
            console.error('Processed audio pitch analysis error:', error);
            this.setStatus('분석 실패');
        }
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
            this.player.play(this.originalAudio);
        }
    }

    playProcessed() {
        if (this.processedAudio) {
            this.player.play(this.processedAudio);
        }
    }

    stopProcessed() {
        this.player.stop();
    }

    downloadProcessed() {
        if (!this.processedAudio) return;

        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const buffer = audioContext.createBuffer(
            1,
            this.processedAudio.getLength(),
            this.processedAudio.getSampleRate()
        );

        const channelData = buffer.getChannelData(0);
        const data = this.processedAudio.getData();
        for (let i = 0; i < data.length; i++) {
            channelData[i] = data[i];
        }

        // TODO: Convert to WAV and download
        alert('다운로드 기능은 준비 중입니다.');
    }

    setStatus(text) {
        document.getElementById('statusText').textContent = text;
    }
}

// Initialize app
const app = new UIController();
window.app = app;
