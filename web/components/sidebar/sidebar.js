/**
 * Sidebar Component
 * Handles effect controls, generate, playback and download actions
 */
export class Sidebar {
    constructor(options = {}) {
        this.onApplyEffects = options.onApplyEffects || (() => { });
        this.onPlayProcessed = options.onPlayProcessed || (() => { });
        this.onStopProcessed = options.onStopProcessed || (() => { });
        this.onDownload = options.onDownload || (() => { });
        this.onEffectChange = options.onEffectChange || (() => { });

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
            filterHelpText: document.getElementById('filterHelpText'),

            // Reverse
            reversePlayback: document.getElementById('reversePlayback'),

            // Action buttons
            applyAllEffects: document.getElementById('applyAllEffects'),
            resetEffects: document.getElementById('resetEffects'),
            playProcessed: document.getElementById('playProcessed'),
            stopProcessed: document.getElementById('stopProcessed'),
            downloadProcessed: document.getElementById('downloadProcessed'),

            // Status
            statusText: document.getElementById('statusText')
        };

        this.bindEvents();
        this.updateUI();
        // 초기 필터 설명 업데이트
        if (this.elements.filterType) {
            this.updateFilterHelpText(this.elements.filterType.value);
        }
    }

    bindEvents() {
        const {
            pitchShift, pitchQuality,
            timeStretch, timeStretchQuality,
            filterType, filterParam1, filterParam2,
            reversePlayback,
            applyAllEffects, resetEffects, playProcessed, stopProcessed, downloadProcessed
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
                const filterValue = e.target.value;
                this.toggleFilterParams(filterValue !== 'none');
                this.updateFilterHelpText(filterValue);
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

        if (resetEffects) {
            resetEffects.addEventListener('click', () => this.resetEffects());
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

    updateFilterHelpText(filterType) {
        if (!this.elements.filterHelpText) return;

        const helpTexts = {
            '0': {
                title: '☕ 컵 속 목소리',
                param1: '1번 슬라이더: 저음(둔탁함) 양 조절',
                param2: '2번 슬라이더: 고음 잘라내는 정도 (값이 클수록 더 먹먹함)'
            },
            '1': {
                title: '📡 무전기',
                param1: '1번 슬라이더: 전화기/무전기 톤 강도 (중간 음만 남기는 정도)',
                param2: '2번 슬라이더: 소리 선명도 (값이 작을수록 더 깎인 느낌)'
            },
            '3': {
                title: '🤖 로봇 목소리',
                param1: '1번 슬라이더: (현재 버전에서는 사용하지 않음)',
                param2: '2번 슬라이더: (현재 버전에서는 사용하지 않음)'
            },
            '4': {
                title: '🌊 메아리 (Echo)',
                param1: '1번 슬라이더: 메아리 간격 (값이 클수록 더 느리게 울림)',
                param2: '2번 슬라이더: 메아리 세기 (값이 클수록 뒤에 오는 메아리가 더 크게 남음)'
            },
            '5': {
                title: '🏛 잔향 (Reverb)',
                param1: '1번 슬라이더: 공간 크기 / 잔향 길이',
                param2: '2번 슬라이더: 벽 흡음 정도 (값이 작을수록 잔향이 오래 남음)'
            },
            '6': {
                title: '🎸 기타 앰프 (왜곡 효과)',
                param1: '1번 슬라이더: 왜곡 강도 (값이 클수록 더 강하게 왜곡)',
                param2: '2번 슬라이더: 톤 조절 (값이 클수록 밝은 소리)'
            },
            '7': {
                title: '📻 AM 라디오 (노이즈 + 대역 제한)',
                param1: '1번 슬라이더: 노이즈 양 (값이 클수록 더 많은 노이즈)',
                param2: '2번 슬라이더: 대역폭 (값이 클수록 더 넓은 주파수 대역)'
            },
            '8': {
                title: '🎵 합창 효과 (Chorus)',
                param1: '1번 슬라이더: 변조 속도 (값이 클수록 더 빠르게 변조)',
                param2: '2번 슬라이더: 효과 깊이 (값이 클수록 더 강한 합창 효과)'
            },
            '9': {
                title: '🌊 플랜저 (Flanger)',
                param1: '1번 슬라이더: 변조 속도 (값이 클수록 더 빠르게 변조)',
                param2: '2번 슬라이더: 효과 깊이 (값이 클수록 더 강한 플랜저 효과)'
            },
            '10': {
                title: '📺 뉴스 인터뷰 목소리',
                param1: '1번 슬라이더: 변환 강도 (값이 클수록 더 여성 목소리에 가까워짐)',
                param2: '2번 슬라이더: (사용하지 않음)'
            },
            '11': {
                title: '🎭 범인 목소리',
                param1: '1번 슬라이더: 변환 강도 (값이 클수록 더 낮고 둔한 목소리)',
                param2: '2번 슬라이더: (사용하지 않음)'
            }
        };

        const help = helpTexts[filterType];
        if (help) {
            this.elements.filterHelpText.innerHTML = `
                <div><strong>${help.title}</strong> 선택 시</div>
                <div>• ${help.param1}</div>
                <div>• ${help.param2}</div>
            `;
        } else {
            this.elements.filterHelpText.innerHTML = '';
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
            // Add processing class for visual feedback
            if (this.state.isProcessing) {
                this.elements.statusText.classList.add('processing-status');
            } else {
                this.elements.statusText.classList.remove('processing-status');
            }
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

    resetEffects() {
        // 필터, 피치, 스피드만 초기화 (오디오는 유지)
        // Reset pitch
        if (this.elements.pitchShift) {
            this.elements.pitchShift.value = 0;
            this.updatePitchDisplay(0);
        }

        // Reset time stretch
        if (this.elements.timeStretch) {
            this.elements.timeStretch.value = 1.0;
            this.updateTimeDisplay(1.0);
        }

        // Reset filter
        if (this.elements.filterType) {
            this.elements.filterType.value = 'none';
            this.toggleFilterParams(false);
            this.updateFilterHelpText('none');
        }

        // Reset filter params
        if (this.elements.filterParam1) {
            this.elements.filterParam1.value = 0.5;
        }
        if (this.elements.filterParam2) {
            this.elements.filterParam2.value = 0.5;
        }

        // Reset reverse
        if (this.elements.reversePlayback) {
            this.elements.reversePlayback.checked = false;
        }

        // 효과 변경 알림
        this.onEffectChange(this.getEffectValues());
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
            this.updateFilterHelpText('none');
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
