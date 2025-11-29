/**
 * Toolbar Component
 * Handles recording, file upload, and playback controls
 */
export class Toolbar {
    constructor(options = {}) {
        this.onRecord = options.onRecord || (() => { });
        this.onStopRecord = options.onStopRecord || (() => { });
        this.onFileUpload = options.onFileUpload || (() => { });
        this.onPlayOriginal = options.onPlayOriginal || (() => { });

        this.isRecording = false;
        this.hasAudio = false;

        this.elements = {};
        this.init();
    }

    init() {
        this.elements = {
            startRecord: document.getElementById('startRecord'),
            stopRecord: document.getElementById('stopRecord'),
            uploadFile: document.getElementById('uploadFile'),
            fileInput: document.getElementById('fileInput'),
            fileInfo: document.getElementById('fileInfo'),
            playOriginal: document.getElementById('playOriginal')
        };

        this.bindEvents();
    }

    bindEvents() {
        const { startRecord, stopRecord, uploadFile, fileInput, playOriginal } = this.elements;

        if (startRecord) {
            startRecord.addEventListener('click', () => this.startRecording());
        }

        if (stopRecord) {
            stopRecord.addEventListener('click', () => this.stopRecording());
        }

        if (uploadFile) {
            uploadFile.addEventListener('click', () => fileInput?.click());
        }

        if (fileInput) {
            fileInput.addEventListener('change', (e) => this.handleFileUpload(e));
        }

        if (playOriginal) {
            playOriginal.addEventListener('click', () => this.onPlayOriginal());
        }
    }

    startRecording() {
        this.isRecording = true;
        this.updateUI();
        this.onRecord();
    }

    stopRecording() {
        this.isRecording = false;
        this.updateUI();
        this.onStopRecord();
    }

    // 외부에서 UI만 녹음 상태로/해제로 바꿀 때 사용 (콜백은 호출 안 함)
    setRecordingState(isRecording) {
        this.isRecording = isRecording;
        this.updateUI();
    }

    handleFileUpload(event) {
        const file = event.target.files[0];
        if (!file) return;

        this.setFileName(file.name);
        this.onFileUpload(file);
    }

    setFileName(name) {
        if (this.elements.fileInfo) {
            this.elements.fileInfo.textContent = name;
        }
    }

    setAudioLoaded(loaded) {
        this.hasAudio = loaded;
        this.updateUI();
    }

    updateUI() {
        const { startRecord, stopRecord, playOriginal } = this.elements;

        if (startRecord) {
            startRecord.disabled = this.isRecording;
        }

        if (stopRecord) {
            stopRecord.disabled = !this.isRecording;
        }

        if (playOriginal) {
            playOriginal.disabled = !this.hasAudio;
        }
    }

    setStatus(text) {
        if (this.elements.fileInfo) {
            this.elements.fileInfo.textContent = text;
        }
    }
}
