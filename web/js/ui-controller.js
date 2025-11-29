import { AudioRecorder } from './audio-recorder.js';
import { AudioPlayer } from './audio-player.js';
import { Toolbar } from '../components/toolbar/toolbar.js';
import { Sidebar } from '../components/sidebar/sidebar.js';
import { PitchChart } from '../components/chart/chart.js';

/**
 * UIController - Main application controller for Adobe-style UI
 * Coordinates between toolbar, sidebar, and chart components
 */
export class UIController {
    constructor() {
        // Core audio handling
        this.recorder = null;
        this.player = new AudioPlayer();
        this.module = null;

        // Audio data
        this.originalAudio = null;
        this.processedAudio = null;
        this.sampleRate = 48000;

        // Components
        this.toolbar = null;
        this.sidebar = null;
        this.chart = null;

        // Algorithm settings (external libraries only)
        this.currentPitchAlgorithm = 'soundtouch';
        this.currentDurationAlgorithm = 'soundtouch';

        // Playback state
        this.isPlaying = false;
    }

    async init() {
        // Wait for WASM module
        if (typeof Module === 'undefined') {
            console.error('Module is not defined! main.js may not have loaded properly.');
            return;
        }

        this.module = Module;
        this.recorder = new AudioRecorder(this.module);
        this.module.init();

        // Initialize components
        this.initComponents();
        this.setupEventListeners();

        console.log('UIController initialized');
    }

    initComponents() {
        // Initialize Toolbar
        this.toolbar = new Toolbar({
            onRecord: () => this.startRecording(),
            onStopRecord: () => this.stopRecording(),
            onFileUpload: (file) => this.handleFileUpload(file),
            onPlayOriginal: () => this.playOriginal()
        });

        // Initialize Sidebar
        this.sidebar = new Sidebar({
            onApplyEffects: (effects) => this.applyEffects(effects),
            onPlayProcessed: () => this.playProcessed(),
            onStopProcessed: () => this.stopProcessed(),
            onDownload: () => this.downloadProcessed(),
            onEffectChange: (effects) => this.onEffectChange(effects)
        });

        // Initialize Chart
        this.chart = new PitchChart('pitch-chart', {
            animate: true,
            showPoints: false,
            showGrid: true
        });
    }

    setupEventListeners() {
        // Quality select changes
        const pitchQuality = document.getElementById('pitchQuality');
        if (pitchQuality) {
            pitchQuality.addEventListener('change', (e) => {
                this.currentPitchAlgorithm = e.target.value;
            });
        }

        const timeStretchQuality = document.getElementById('timeStretchQuality');
        if (timeStretchQuality) {
            timeStretchQuality.addEventListener('change', (e) => {
                this.currentDurationAlgorithm = e.target.value;
            });
        }
    }

    // ==================== Recording ====================

    async startRecording() {
        this.originalAudio = null;
        this.processedAudio = null;
        this.chart?.clear();
        this.sidebar?.setAudioLoaded(false);
        this.sidebar?.setStatus('Recording...');

        try {
            await this.recorder.startRecording();

            // 실제로 녹음이 시작된 시점에서 확실히 UI를 녹음 상태로 맞춰줌
            this.toolbar?.setRecordingState(true);
        } catch (error) {
            console.error('Recording failed:', error);

            // 권한 관련 에러인지 판별
            const message = error?.message || '';
            const name = error?.name || '';
            const isPermissionError =
                name === 'NotAllowedError' ||
                name === 'SecurityError' ||
                message.includes('권한') ||
                message.toLowerCase().includes('permission');

            // 실패했으므로 UI도 다시 녹음 전 상태로 돌림
            this.toolbar?.setRecordingState(false);

            if (isPermissionError) {
                this.sidebar?.setStatus('Microphone access denied');
                this.showModal('마이크 권한이 필요합니다.\n\n브라우저 설정에서 마이크 접근을 허용해 주세요.');
            } else {
                this.sidebar?.setStatus('Recording failed');
            }
        }
    }

    stopRecording() {
        this.originalAudio = this.recorder.stopRecording();
        this.toolbar?.setAudioLoaded(true);
        this.sidebar?.setAudioLoaded(true);
        this.sidebar?.setStatus('Recording complete');

        // Analyze pitch automatically
        this.analyzePitch();
    }

    // ==================== File Upload ====================

    async handleFileUpload(file) {
        this.sidebar?.setStatus('Loading file...');
        this.chart?.showLoading('Loading...');

        try {
            const arrayBuffer = await file.arrayBuffer();
            const audioData = new Uint8Array(arrayBuffer);

            // Extract sample rate from WAV header
            const dataView = new DataView(arrayBuffer);
            this.sampleRate = dataView.getUint32(24, true);

            this.originalAudio = audioData;
            this.processedAudio = null;

            this.toolbar?.setAudioLoaded(true);
            this.toolbar?.setFileName(file.name);
            this.sidebar?.setAudioLoaded(true);
            this.sidebar?.setStatus('File loaded');

            // Analyze pitch automatically
            await this.analyzePitch();
        } catch (error) {
            console.error('File upload failed:', error);
            this.sidebar?.setStatus('Failed to load file');
            this.chart?.hideLoading();
        }
    }

    // ==================== Pitch Analysis ====================

    async analyzePitch() {
        if (!this.originalAudio) return;

        this.chart?.showLoading('Analyzing pitch...');
        this.sidebar?.setStatus('Analyzing...');

        try {
            // Convert to Float32Array if needed
            let audioData;
            if (this.originalAudio instanceof Float32Array) {
                audioData = this.originalAudio;
            } else {
                audioData = this.wavToFloat32(this.originalAudio);
            }

            // Analyze pitch using WASM
            const pitchData = await this.analyzeWithWasm(audioData);

            // Display in chart with "Original" label
            this.chart?.hideLoading();
            this.chart?.setData(pitchData, 'Original');
            this.sidebar?.setStatus('Ready');
        } catch (error) {
            console.error('Pitch analysis failed:', error);
            this.chart?.hideLoading();
            this.sidebar?.setStatus('Analysis failed');
        }
    }

    async analyzeWithWasm(audioData) {
        const length = audioData.length;

        // Allocate memory in WASM heap
        const ptr = this.module._malloc(length * 4);
        this.module.HEAPF32.set(audioData, ptr >> 2);

        // Call WASM pitch analysis (Emscripten bindings return JS array directly)
        const result = this.module.analyzePitch(ptr, length, this.sampleRate);

        // Free allocated memory
        this.module._free(ptr);

        // Convert result to chart format
        const pitchData = [];
        if (result && result.length) {
            for (let i = 0; i < result.length; i++) {
                const point = result[i];
                if (point.frequency > 0) {
                    pitchData.push({
                        time: point.time,
                        pitch: point.frequency
                    });
                }
            }
        }

        return pitchData;
    }

    async analyzeProcessedAudio() {
        if (!this.processedAudio) return;

        this.chart?.showLoading('Analyzing processed audio...');
        this.sidebar?.setStatus('Analyzing processed audio...');

        try {
            // Analyze pitch using WASM
            const pitchData = await this.analyzeWithWasm(this.processedAudio);

            // Display in chart with "Processed" label
            this.chart?.hideLoading();
            this.chart?.setData(pitchData, 'Processed');
            this.sidebar?.setStatus('Ready to play');
        } catch (error) {
            console.error('Processed audio pitch analysis failed:', error);
            this.chart?.hideLoading();
            this.sidebar?.setStatus('Analysis failed');
        }
    }

    // ==================== Effects Processing ====================

    onEffectChange(effects) {
        // Called when any effect slider/control changes
        // Can be used for real-time preview in the future
    }

    async applyEffects(effects) {
        if (!this.originalAudio) {
            alert('Please record or upload audio first.');
            return;
        }

        this.player.stop();
        this.sidebar?.setStatus('Processing...');

        try {
            // Start with original audio
            let audioData = this.originalAudio instanceof Float32Array
                ? this.originalAudio
                : this.wavToFloat32(this.originalAudio);

            // Apply pitch shift
            if (Math.abs(effects.pitch.semitones) > 0.01) {
                audioData = await this.applyPitchShift(audioData, effects.pitch);
            }

            // Apply time stretch
            if (Math.abs(effects.timeStretch.ratio - 1.0) > 0.01) {
                audioData = await this.applyTimeStretch(audioData, effects.timeStretch);
            }

            // Apply filter
            if (effects.filter.type !== 'none') {
                audioData = await this.applyFilter(audioData, effects.filter);
            }

            // Apply reverse
            if (effects.reverse) {
                audioData = this.reverseAudio(audioData);
            }

            this.processedAudio = audioData;
            this.sidebar?.setProcessed(true);
            this.sidebar?.setStatus('Processing complete');

            // Analyze pitch of processed audio and update chart
            await this.analyzeProcessedAudio();
        } catch (error) {
            console.error('Effects processing failed:', error);
            this.sidebar?.setProcessing(false);
            this.sidebar?.setStatus('Processing failed');
            alert('Failed to process audio: ' + error.message);
        }
    }

    async applyPitchShift(audioData, pitchConfig) {
        const length = audioData.length;
        const ptr = this.module._malloc(length * 4);
        this.module.HEAPF32.set(audioData, ptr >> 2);

        const algorithm = pitchConfig.algorithm || this.currentPitchAlgorithm;

        // Call WASM function (returns Float32Array directly)
        const result = this.module.applyUniformPitchShift(
            ptr, length, this.sampleRate,
            pitchConfig.semitones,
            algorithm
        );

        this.module._free(ptr);

        // Convert to Float32Array if needed
        if (result instanceof Float32Array) {
            return result;
        }
        return new Float32Array(result);
    }

    async applyTimeStretch(audioData, stretchConfig) {
        const length = audioData.length;
        const ptr = this.module._malloc(length * 4);
        this.module.HEAPF32.set(audioData, ptr >> 2);

        const algorithm = stretchConfig.algorithm || this.currentDurationAlgorithm;

        // Call WASM function (returns Float32Array directly)
        const result = this.module.applyUniformTimeStretch(
            ptr, length, this.sampleRate,
            stretchConfig.ratio,
            algorithm
        );

        this.module._free(ptr);

        // Convert to Float32Array if needed
        if (result instanceof Float32Array) {
            return result;
        }
        return new Float32Array(result);
    }

    async applyFilter(audioData, filterConfig) {
        const length = audioData.length;
        const ptr = this.module._malloc(length * 4);
        this.module.HEAPF32.set(audioData, ptr >> 2);

        const filterType = parseInt(filterConfig.type);

        // Call WASM function (returns typed_memory_view)
        const resultView = this.module.applyVoiceFilter(
            ptr, length, this.sampleRate,
            filterType,
            filterConfig.param1,
            filterConfig.param2
        );

        // Copy the result since typed_memory_view is a view into WASM memory
        const result = new Float32Array(resultView.length);
        for (let i = 0; i < resultView.length; i++) {
            result[i] = resultView[i];
        }

        this.module._free(ptr);

        return result;
    }

    reverseAudio(audioData) {
        const length = audioData.length;
        const ptr = this.module._malloc(length * 4);
        this.module.HEAPF32.set(audioData, ptr >> 2);

        // Call WASM function (returns Float32Array directly)
        const result = this.module.reverseAudio(ptr, length, this.sampleRate);

        this.module._free(ptr);

        // Convert to Float32Array if needed
        if (result instanceof Float32Array) {
            return result;
        }
        return new Float32Array(result);
    }

    // 간단한 인앱 모달 표시 헬퍼
    showModal(message) {
        let overlay = document.getElementById('app-modal-overlay');
        if (!overlay) {
            overlay = document.createElement('div');
            overlay.id = 'app-modal-overlay';
            overlay.style.position = 'fixed';
            overlay.style.inset = '0';
            overlay.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
            overlay.style.display = 'flex';
            overlay.style.alignItems = 'center';
            overlay.style.justifyContent = 'center';
            overlay.style.zIndex = '9999';

            const modal = document.createElement('div');
            modal.id = 'app-modal';
            modal.style.backgroundColor = 'var(--bg-secondary, #252526)';
            modal.style.color = 'var(--text-primary, #cccccc)';
            modal.style.border = '1px solid var(--border-color, #3e3e42)';
            modal.style.borderRadius = '8px';
            modal.style.padding = '20px 24px';
            modal.style.minWidth = '260px';
            modal.style.maxWidth = '360px';
            modal.style.boxShadow = '0 12px 30px rgba(0, 0, 0, 0.6)';

            const textEl = document.createElement('div');
            textEl.id = 'app-modal-text';
            textEl.style.whiteSpace = 'pre-line';
            textEl.style.fontSize = '14px';
            textEl.style.marginBottom = '16px';

            const buttonRow = document.createElement('div');
            buttonRow.style.display = 'flex';
            buttonRow.style.justifyContent = 'flex-end';

            const closeBtn = document.createElement('button');
            closeBtn.textContent = '확인';
            closeBtn.style.backgroundColor = 'var(--accent-color, #0078d4)';
            closeBtn.style.color = '#ffffff';
            closeBtn.style.border = 'none';
            closeBtn.style.borderRadius = '4px';
            closeBtn.style.padding = '6px 14px';
            closeBtn.style.cursor = 'pointer';
            closeBtn.style.fontSize = '13px';

            const hide = () => {
                overlay.style.display = 'none';
            };

            closeBtn.addEventListener('click', hide);
            overlay.addEventListener('click', (e) => {
                if (e.target === overlay) hide();
            });

            buttonRow.appendChild(closeBtn);
            modal.appendChild(textEl);
            modal.appendChild(buttonRow);
            overlay.appendChild(modal);
            document.body.appendChild(overlay);
        }

        const textEl = document.getElementById('app-modal-text');
        if (textEl) {
            textEl.textContent = message;
        }

        overlay.style.display = 'flex';
    }

    // ==================== Playback ====================

    async playOriginal() {
        if (!this.originalAudio) return;

        try {
            this.isPlaying = true;
            if (this.originalAudio instanceof Float32Array) {
                await this.player.playFloat32Array(this.originalAudio, this.sampleRate);
            } else {
                await this.player.playWavData(this.originalAudio);
            }
        } catch (error) {
            console.error('Playback failed:', error);
        } finally {
            this.isPlaying = false;
        }
    }

    async playProcessed() {
        if (!this.processedAudio) return;

        try {
            this.isPlaying = true;
            await this.player.playFloat32Array(this.processedAudio, this.sampleRate);
        } catch (error) {
            console.error('Playback failed:', error);
        } finally {
            this.isPlaying = false;
        }
    }

    stopProcessed() {
        this.player.stop();
        this.isPlaying = false;
    }

    // ==================== Download ====================

    downloadProcessed() {
        if (!this.processedAudio) return;

        const wavData = this.float32ToWav(this.processedAudio);
        this.player.downloadWav(wavData, 'processed.wav');
    }

    // ==================== Audio Conversion Utilities ====================

    wavToFloat32(wavData) {
        const dataView = new DataView(wavData.buffer || wavData);
        const samples = [];

        // Skip WAV header (44 bytes)
        for (let i = 44; i < wavData.length; i += 2) {
            const sample = dataView.getInt16(i, true) / 32768.0;
            samples.push(sample);
        }

        return new Float32Array(samples);
    }

    float32ToWav(audioData) {
        const numChannels = 1;
        const bitsPerSample = 16;
        const bytesPerSample = bitsPerSample / 8;
        const blockAlign = numChannels * bytesPerSample;
        const byteRate = this.sampleRate * blockAlign;
        const dataSize = audioData.length * bytesPerSample;
        const bufferSize = 44 + dataSize;

        const buffer = new ArrayBuffer(bufferSize);
        const view = new DataView(buffer);

        // WAV header
        this.writeString(view, 0, 'RIFF');
        view.setUint32(4, bufferSize - 8, true);
        this.writeString(view, 8, 'WAVE');
        this.writeString(view, 12, 'fmt ');
        view.setUint32(16, 16, true);
        view.setUint16(20, 1, true);
        view.setUint16(22, numChannels, true);
        view.setUint32(24, this.sampleRate, true);
        view.setUint32(28, byteRate, true);
        view.setUint16(32, blockAlign, true);
        view.setUint16(34, bitsPerSample, true);
        this.writeString(view, 36, 'data');
        view.setUint32(40, dataSize, true);

        // Audio data
        let offset = 44;
        for (let i = 0; i < audioData.length; i++) {
            const sample = Math.max(-1, Math.min(1, audioData[i]));
            view.setInt16(offset, sample * 32767, true);
            offset += 2;
        }

        return new Uint8Array(buffer);
    }

    writeString(view, offset, string) {
        for (let i = 0; i < string.length; i++) {
            view.setUint8(offset + i, string.charCodeAt(i));
        }
    }
}
