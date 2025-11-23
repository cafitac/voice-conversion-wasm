/**
 * Sidebar Component
 * Handles effect controls, generate, playback and download actions
 */
export class Sidebar {
    constructor(options = {}) {
        this.onApplyEffects = options.onApplyEffects || (() => {});
        this.onPlayProcessed = options.onPlayProcessed || (() => {});
        this.onStopProcessed = options.onStopProcessed || (() => {});
        this.onDownload = options.onDownload || (() => {});
        this.onEffectChange = options.onEffectChange || (() => {});

        this.elements = {};
        this.state = {
            hasAudio: false,
            hasProcessed: false,
            isProcessing: false
        };

        this.init();
    }

    init() {
        this.elements = {
            // Pitch controls
            pitchShift: document.getElementById('pitchShift'),
            pitchValue: document.getElementById('pitchValue'),
            pitchQuality: document.getElementById('pitchQuality'),

            // Time stretch controls
            timeStretch: document.getElementById('timeStretch'),
            timeValue: document.getElementById('timeValue'),
            timeStretchQuality: document.getElementById('timeStretchQuality'),

            // Filter controls
            filterType: document.getElementById('filterType'),
            filterParams: document.getElementById('filterParams'),
            filterParam1: document.getElementById('filterParam1'),
            filterParam2: document.getElementById('filterParam2'),

            // Reverse
            reversePlayback: document.getElementById('reversePlayback'),

            // Action buttons
            applyAllEffects: document.getElementById('applyAllEffects'),
            playProcessed: document.getElementById('playProcessed'),
            stopProcessed: document.getElementById('stopProcessed'),
            downloadProcessed: document.getElementById('downloadProcessed'),

            // Status
            statusText: document.getElementById('statusText')
        };

        this.bindEvents();
        this.updateUI();
    }

    bindEvents() {
        const {
            pitchShift, pitchQuality,
            timeStretch, timeStretchQuality,
            filterType, filterParam1, filterParam2,
            reversePlayback,
            applyAllEffects, playProcessed, stopProcessed, downloadProcessed
        } = this.elements;

        // Pitch slider
        if (pitchShift) {
            pitchShift.addEventListener('input', (e) => {
                this.updatePitchDisplay(e.target.value);
                this.onEffectChange(this.getEffectValues());
            });
        }

        // Time stretch slider
        if (timeStretch) {
            timeStretch.addEventListener('input', (e) => {
                this.updateTimeDisplay(e.target.value);
                this.onEffectChange(this.getEffectValues());
            });
        }

        // Filter type change
        if (filterType) {
            filterType.addEventListener('change', (e) => {
                this.toggleFilterParams(e.target.value !== 'none');
                this.onEffectChange(this.getEffectValues());
            });
        }

        // Filter params
        [filterParam1, filterParam2].forEach(param => {
            if (param) {
                param.addEventListener('input', () => {
                    this.onEffectChange(this.getEffectValues());
                });
            }
        });

        // Algorithm selects
        [pitchQuality, timeStretchQuality].forEach(select => {
            if (select) {
                select.addEventListener('change', () => {
                    this.onEffectChange(this.getEffectValues());
                });
            }
        });

        // Reverse checkbox
        if (reversePlayback) {
            reversePlayback.addEventListener('change', () => {
                this.onEffectChange(this.getEffectValues());
            });
        }

        // Action buttons
        if (applyAllEffects) {
            applyAllEffects.addEventListener('click', () => this.applyEffects());
        }

        if (playProcessed) {
            playProcessed.addEventListener('click', () => this.onPlayProcessed());
        }

        if (stopProcessed) {
            stopProcessed.addEventListener('click', () => this.onStopProcessed());
        }

        if (downloadProcessed) {
            downloadProcessed.addEventListener('click', () => this.onDownload());
        }
    }

    updatePitchDisplay(value) {
        if (this.elements.pitchValue) {
            const num = parseFloat(value);
            this.elements.pitchValue.textContent = num > 0 ? `+${num}` : num;
        }
    }

    updateTimeDisplay(value) {
        if (this.elements.timeValue) {
            this.elements.timeValue.textContent = `${parseFloat(value).toFixed(1)}x`;
        }
    }

    toggleFilterParams(show) {
        if (this.elements.filterParams) {
            this.elements.filterParams.classList.toggle('hidden', !show);
        }
    }

    getEffectValues() {
        return {
            pitch: {
                semitones: parseFloat(this.elements.pitchShift?.value || 0),
                algorithm: this.elements.pitchQuality?.value || 'phase-vocoder'
            },
            timeStretch: {
                ratio: parseFloat(this.elements.timeStretch?.value || 1.0),
                algorithm: this.elements.timeStretchQuality?.value || 'soundtouch'
            },
            filter: {
                type: this.elements.filterType?.value || 'none',
                param1: parseFloat(this.elements.filterParam1?.value || 0.5),
                param2: parseFloat(this.elements.filterParam2?.value || 0.5)
            },
            reverse: this.elements.reversePlayback?.checked || false
        };
    }

    applyEffects() {
        this.setProcessing(true);
        this.onApplyEffects(this.getEffectValues());
    }

    setAudioLoaded(loaded) {
        this.state.hasAudio = loaded;
        this.updateUI();
    }

    setProcessed(processed) {
        this.state.hasProcessed = processed;
        this.state.isProcessing = false;
        this.updateUI();
    }

    setProcessing(processing) {
        this.state.isProcessing = processing;
        this.updateUI();
    }

    setStatus(text) {
        if (this.elements.statusText) {
            this.elements.statusText.textContent = text;
        }
    }

    updateUI() {
        const { hasAudio, hasProcessed, isProcessing } = this.state;
        const { applyAllEffects, playProcessed, stopProcessed, downloadProcessed } = this.elements;

        if (applyAllEffects) {
            applyAllEffects.disabled = !hasAudio || isProcessing;
            if (isProcessing) {
                applyAllEffects.innerHTML = '<span class="icon">⏳</span> Processing...';
                applyAllEffects.classList.add('processing');
            } else {
                applyAllEffects.innerHTML = '<span class="icon">✨</span> Generate';
                applyAllEffects.classList.remove('processing');
            }
        }

        if (playProcessed) {
            playProcessed.disabled = !hasProcessed;
        }

        if (stopProcessed) {
            stopProcessed.disabled = !hasProcessed;
        }

        if (downloadProcessed) {
            downloadProcessed.disabled = !hasProcessed;
        }

        // 처리 완료 시 시각적 피드백
        if (hasProcessed && !isProcessing) {
            this.showCompletionFeedback();
        }
    }

    showCompletionFeedback() {
        // 상태 텍스트 강조
        if (this.elements.statusText) {
            this.elements.statusText.style.color = 'var(--success-color)';
            this.elements.statusText.textContent = '✓ Complete - Ready to play';

            // 3초 후 원래 색상으로 복원
            setTimeout(() => {
                if (this.elements.statusText) {
                    this.elements.statusText.style.color = '';
                }
            }, 3000);
        }

        // 재생 버튼 강조 애니메이션
        if (this.elements.playProcessed) {
            this.elements.playProcessed.classList.add('highlight');
            setTimeout(() => {
                this.elements.playProcessed?.classList.remove('highlight');
            }, 2000);
        }
    }

    reset() {
        // Reset sliders
        if (this.elements.pitchShift) {
            this.elements.pitchShift.value = 0;
            this.updatePitchDisplay(0);
        }

        if (this.elements.timeStretch) {
            this.elements.timeStretch.value = 1.0;
            this.updateTimeDisplay(1.0);
        }

        // Reset filter
        if (this.elements.filterType) {
            this.elements.filterType.value = 'none';
            this.toggleFilterParams(false);
        }

        // Reset reverse
        if (this.elements.reversePlayback) {
            this.elements.reversePlayback.checked = false;
        }

        // Reset state
        this.state = {
            hasAudio: false,
            hasProcessed: false,
            isProcessing: false
        };
        this.updateUI();
    }
}
