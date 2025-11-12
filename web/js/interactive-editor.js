/**
 * InteractiveEditor - D3.js 기반 인터랙티브 Pitch/Duration 편집
 */
export class InteractiveEditor {
    constructor(containerId) {
        this.containerId = containerId;
        this.container = document.getElementById(containerId);
        this.frameData = [];
        this.pitchEdits = [];  // { frameIndex, semitones } - 모든 프레임
        this.keyPoints = [];   // { frameIndex, semitones } - 편집 가능한 key points만
        this.durationRegions = [];  // { startFrame, endFrame, ratio }

        this.margin = { top: 20, right: 30, bottom: 50, left: 80 };
        this.width = 0;
        this.height = 0;

        this.svg = null;
        this.pitchChart = null;
        this.durationChart = null;
        this.brush = null;

        // Key points 간격 (프레임 수)
        this.keyPointInterval = 15;  // 매 15프레임마다 하나의 key point

        // 드래그 영향 범위 (프레임 수)
        this.influenceRadius = 30;  // 주변 ±30프레임 영향
    }

    /**
     * 프레임 데이터 설정 및 차트 생성
     */
    setFrameData(frames) {
        this.frameData = frames;
        this.pitchEdits = frames.map((f, i) => ({ frameIndex: i, semitones: 0 }));

        // Key points 생성 (매 keyPointInterval 프레임마다 + 시작/끝)
        this.keyPoints = [];
        this.keyPoints.push({ frameIndex: 0, semitones: 0 });  // 시작점

        for (let i = this.keyPointInterval; i < frames.length; i += this.keyPointInterval) {
            this.keyPoints.push({ frameIndex: i, semitones: 0 });
        }

        // 마지막 프레임이 key point가 아니면 추가
        if (this.keyPoints[this.keyPoints.length - 1].frameIndex !== frames.length - 1) {
            this.keyPoints.push({ frameIndex: frames.length - 1, semitones: 0 });
        }

        console.log(`[InteractiveEditor] Key points 생성: ${this.keyPoints.length}개 (전체 ${frames.length} 프레임)`);

        this.durationRegions = [];
        this.render();
    }

    /**
     * 전체 차트 렌더링
     */
    render() {
        console.log('[InteractiveEditor] render() 시작', {
            container: !!this.container,
            frameDataLength: this.frameData.length,
            pitchEditsLength: this.pitchEdits.length
        });

        if (!this.container) {
            console.error('[InteractiveEditor] container가 없습니다!');
            return;
        }

        if (this.frameData.length === 0) {
            console.warn('[InteractiveEditor] frameData가 비어있습니다!');
            return;
        }

        // 컨테이너 초기화
        this.container.innerHTML = '';

        // 크기 계산
        const containerWidth = this.container.clientWidth || 800;
        this.width = containerWidth - this.margin.left - this.margin.right;
        this.height = 500 - this.margin.top - this.margin.bottom;

        console.log('[InteractiveEditor] 차트 크기:', { width: this.width, height: this.height });

        // Pitch 차트 생성
        this.renderPitchChart();

        // Duration 차트 생성
        this.renderDurationChart();

        console.log('[InteractiveEditor] render() 완료');
    }

    /**
     * Pitch 차트 렌더링 (Key points + 곡선 보간 + Zoom/Pan)
     */
    renderPitchChart() {
        console.log('[InteractiveEditor] renderPitchChart() 시작');

        const pitchDiv = document.createElement('div');
        pitchDiv.className = 'chart-section';
        this.container.appendChild(pitchDiv);

        const title = document.createElement('h4');
        title.textContent = `Pitch 편집 (${this.keyPoints.length}개 포인트 | 드래그하면 주변도 영향 | 마우스 휠로 확대/축소)`;
        pitchDiv.appendChild(title);

        // SVG 생성
        const svg = d3.select(pitchDiv)
            .append('svg')
            .attr('width', this.width + this.margin.left + this.margin.right)
            .attr('height', this.height + this.margin.top + this.margin.bottom);

        const g = svg.append('g')
            .attr('transform', `translate(${this.margin.left},${this.margin.top})`);

        // Clip path
        svg.append('defs').append('clipPath')
            .attr('id', 'pitch-clip')
            .append('rect')
            .attr('width', this.width)
            .attr('height', this.height);

        // Scale 설정
        const xScale = d3.scaleLinear()
            .domain([0, this.frameData.length - 1])
            .range([0, this.width]);

        // Smoothed pitch 데이터 생성
        const smoothedPitch = this.getSmoothedPitchData();

        // Y축: 반음 범위 계산 (smoothed 데이터 기준)
        const validPitch = smoothedPitch.filter(p => p !== 0);
        const minPitch = validPitch.length > 0 ? Math.min(...validPitch) : -12;
        const maxPitch = validPitch.length > 0 ? Math.max(...validPitch) : 12;

        // Y축 범위를 더 좁게 설정 (실제 pitch 변화 범위 + 여유분 ±6 semitones)
        const yMin = Math.floor(minPitch - 6);
        const yMax = Math.ceil(maxPitch + 6);

        const yScale = d3.scaleLinear()
            .domain([yMin, yMax])
            .range([this.height, 0]);

        // Zoomable 그룹
        const zoomGroup = g.append('g')
            .attr('clip-path', 'url(#pitch-clip)');

        // X축
        const xAxisGroup = g.append('g')
            .attr('class', 'x-axis')
            .attr('transform', `translate(0,${this.height})`)
            .call(d3.axisBottom(xScale).ticks(10));

        xAxisGroup.append('text')
            .attr('x', this.width / 2)
            .attr('y', 35)
            .attr('fill', 'currentColor')
            .style('text-anchor', 'middle')
            .text('Frame Index');

        // Y축: 계이름으로 표시 (더 적은 tick)
        const yAxisTicks = [];
        for (let s = Math.floor(yMin); s <= Math.ceil(yMax); s += 2) {  // 2 semitones마다
            yAxisTicks.push(s);
        }

        const yAxis = d3.axisLeft(yScale)
            .tickValues(yAxisTicks)
            .tickFormat(d => this.semitoneToNoteName(d));

        g.append('g')
            .call(yAxis)
            .append('text')
            .attr('transform', 'rotate(-90)')
            .attr('x', -this.height / 2)
            .attr('y', -65)
            .attr('fill', 'currentColor')
            .style('text-anchor', 'middle')
            .text('Pitch (계이름, A4=440Hz 기준)');

        // 0 기준선
        let zeroLine = null;
        if (yMin <= 0 && yMax >= 0) {
            zeroLine = zoomGroup.append('line')
                .attr('x1', 0)
                .attr('x2', this.width)
                .attr('y1', yScale(0))
                .attr('y2', yScale(0))
                .attr('stroke', '#666')
                .attr('stroke-dasharray', '4,4');
        }

        // 원본 pitch 라인 (회색, smoothed & 부드러운 곡선)
        const pitchLine = d3.line()
            .defined((d, i) => smoothedPitch[i] !== 0)  // 0이 아닌 값만 연결
            .x((d, i) => xScale(i))
            .y((d, i) => yScale(smoothedPitch[i]))
            .curve(d3.curveBasis);  // Basis spline: 더 부드러운 곡선

        const originalPath = zoomGroup.append('path')
            .datum(this.frameData)
            .attr('fill', 'none')
            .attr('stroke', '#999')
            .attr('stroke-width', 2)
            .attr('opacity', 0.7)
            .attr('d', pitchLine);

        // 편집된 pitch 라인 (파란색, smoothed 기준 + 부드러운 곡선)
        const editedLine = d3.line()
            .defined((d) => smoothedPitch[d.frameIndex] !== 0)
            .x((d) => xScale(d.frameIndex))
            .y(d => yScale(smoothedPitch[d.frameIndex] + d.semitones))
            .curve(d3.curveBasis);  // Basis spline: 더 부드러운 곡선

        const editedPath = zoomGroup.append('path')
            .datum(this.pitchEdits)
            .attr('fill', 'none')
            .attr('stroke', '#3498db')
            .attr('stroke-width', 3)
            .attr('d', editedLine);

        // Key points만 드래그 가능한 점으로 표시
        const self = this;
        const drag = d3.drag()
            .on('drag', function(event, d) {
                // d는 keyPoints의 인덱스
                const keyPointIdx = d;
                const frameIdx = self.keyPoints[keyPointIdx].frameIndex;

                // zoomGroup 기준 상대 좌표 사용 (margin offset 문제 해결)
                const pointer = d3.pointer(event, zoomGroup.node());
                const newY = Math.max(0, Math.min(self.height, pointer[1]));

                // 절대 pitch 값을 원본 대비 shift 값으로 변환
                const absolutePitch = yScale.invert(newY);
                const newSemitones = Math.round((absolutePitch - smoothedPitch[frameIdx]) * 2) / 2;  // 0.5 단위

                // 이 key point만 업데이트 (주변에 영향 없음)
                self.keyPoints[keyPointIdx].semitones = newSemitones;

                // Key points 사이를 보간 (해당 구간만 영향)
                self.interpolateAllFrames();

                // 모든 key points 위치 업데이트 (smoothedPitch 기준)
                keyPointsGroup.selectAll('.key-point')
                    .attr('cy', (d) => {
                        const kpFrameIdx = self.keyPoints[d].frameIndex;
                        return yScale(smoothedPitch[kpFrameIdx] + self.keyPoints[d].semitones);
                    });

                // 곡선 업데이트
                editedPath.datum(self.pitchEdits).attr('d', editedLine);

                // 메인 화면에서 편집 시 버튼 활성화
                if (self.containerId === 'external-chart-container') {
                    const btn = document.getElementById('apply-ext');
                    if (btn) {
                        btn.disabled = false;
                        console.log('[InteractiveEditor] apply-ext 버튼 활성화');
                    }
                } else if (self.containerId === 'highquality-chart-container') {
                    const btn = document.getElementById('apply-hq');
                    if (btn) {
                        btn.disabled = false;
                        console.log('[InteractiveEditor] apply-hq 버튼 활성화');
                    }
                }
            });

        console.log(`[InteractiveEditor] ${this.keyPoints.length}개 key points 표시`);

        // Key points 그룹 (index로 데이터 바인딩)
        const keyPointsGroup = zoomGroup.append('g').attr('class', 'key-points-group');

        const points = keyPointsGroup.selectAll('.key-point')
            .data(d3.range(this.keyPoints.length))  // 인덱스 배열
            .enter()
            .append('circle')
            .attr('class', 'key-point')
            .attr('cx', d => xScale(this.keyPoints[d].frameIndex))
            .attr('cy', d => {
                const frameIdx = this.keyPoints[d].frameIndex;
                return yScale(smoothedPitch[frameIdx] + this.keyPoints[d].semitones);
            })
            .attr('r', 7)
            .attr('fill', '#e74c3c')
            .attr('stroke', '#fff')
            .attr('stroke-width', 2.5)
            .style('cursor', 'ns-resize')
            .call(drag);

        // Tooltip
        points.append('title')
            .text(d => {
                const kp = this.keyPoints[d];
                return `Frame ${kp.frameIndex}\n${this.semitoneToNoteName(smoothedPitch[kp.frameIndex] + kp.semitones)}`;
            });

        console.log('[InteractiveEditor] Key points 렌더링 완료');

        // Zoom 동작
        const zoom = d3.zoom()
            .scaleExtent([1, 50])
            .translateExtent([[0, 0], [this.width, this.height]])
            .extent([[0, 0], [this.width, this.height]])
            .on('zoom', (event) => {
                const newXScale = event.transform.rescaleX(xScale);

                xAxisGroup.call(d3.axisBottom(newXScale).ticks(10));

                originalPath.attr('d', pitchLine.x((d, i) => newXScale(i)));
                editedPath.attr('d', editedLine.x(d => newXScale(d.frameIndex)));

                points.attr('cx', d => newXScale(this.keyPoints[d].frameIndex));

                if (zeroLine) {
                    zeroLine.attr('x2', this.width * event.transform.k);
                }
            });

        svg.call(zoom);
    }

    /**
     * Duration 차트 렌더링 (Brush로 구간 선택 + Zoom/Pan)
     */
    renderDurationChart() {
        const durationDiv = document.createElement('div');
        durationDiv.className = 'chart-section';
        this.container.appendChild(durationDiv);

        const title = document.createElement('h4');
        title.textContent = 'Duration 편집 (브러시로 구간 선택 | 마우스 휠로 확대/축소)';
        durationDiv.appendChild(title);

        // SVG 생성
        const svg = d3.select(durationDiv)
            .append('svg')
            .attr('width', this.width + this.margin.left + this.margin.right)
            .attr('height', this.height + this.margin.top + this.margin.bottom);

        const g = svg.append('g')
            .attr('transform', `translate(${this.margin.left},${this.margin.top})`);

        // Clip path
        svg.append('defs').append('clipPath')
            .attr('id', 'duration-clip')
            .append('rect')
            .attr('width', this.width)
            .attr('height', this.height);

        // Zoomable 그룹
        const zoomGroup = g.append('g')
            .attr('clip-path', 'url(#duration-clip)');

        // Scale 설정
        const xScale = d3.scaleLinear()
            .domain([0, this.frameData.length - 1])
            .range([0, this.width]);

        const yScale = d3.scaleLinear()
            .domain([0, 1])
            .range([this.height, 0]);

        // 축 추가
        const xAxisGroup = g.append('g')
            .attr('class', 'x-axis')
            .attr('transform', `translate(0,${this.height})`)
            .call(d3.axisBottom(xScale).ticks(10));

        xAxisGroup.append('text')
            .attr('x', this.width / 2)
            .attr('y', 35)
            .attr('fill', 'currentColor')
            .style('text-anchor', 'middle')
            .text('Frame Index');

        // RMS 시각화 (막대 그래프)
        const bars = zoomGroup.selectAll('.rms-bar')
            .data(this.frameData)
            .enter()
            .append('rect')
            .attr('class', 'rms-bar')
            .attr('x', (d, i) => xScale(i))
            .attr('y', d => yScale(d.rms))
            .attr('width', Math.max(1, this.width / this.frameData.length))
            .attr('height', d => this.height - yScale(d.rms))
            .attr('fill', d => d.isVoice ? '#2ecc71' : '#95a5a6')
            .attr('opacity', 0.6);

        // Brush 설정
        const self = this;
        const brush = d3.brushX()
            .extent([[0, 0], [this.width, this.height]])
            .on('end', function(event) {
                if (!event.selection) return;

                const currentTransform = d3.zoomTransform(svg.node());
                const newXScale = currentTransform.rescaleX(xScale);

                const [x0, x1] = event.selection;
                const startFrame = Math.round(newXScale.invert(x0));
                const endFrame = Math.round(newXScale.invert(x1));

                // 사용자에게 ratio 입력 받기
                const ratio = prompt(`구간 [${startFrame}, ${endFrame}]의 duration ratio를 입력하세요 (예: 1.5 = 느리게, 0.5 = 빠르게):`, '1.0');
                if (ratio && !isNaN(parseFloat(ratio))) {
                    self.durationRegions.push({
                        startFrame,
                        endFrame,
                        ratio: parseFloat(ratio)
                    });
                    self.updateDurationRegionsDisplay();
                }

                // Brush 초기화
                d3.select(this).call(brush.move, null);
            });

        const brushGroup = g.append('g')
            .attr('class', 'brush')
            .call(brush);

        // Zoom 동작
        const zoom = d3.zoom()
            .scaleExtent([1, 50])
            .translateExtent([[0, 0], [this.width, this.height]])
            .extent([[0, 0], [this.width, this.height]])
            .filter((event) => {
                // Shift 키를 누르고 있으면 zoom, 아니면 brush
                return event.type === 'wheel' || event.shiftKey;
            })
            .on('zoom', (event) => {
                const newXScale = event.transform.rescaleX(xScale);

                // X축 업데이트
                xAxisGroup.call(d3.axisBottom(newXScale).ticks(10));

                // 막대 위치 및 너비 업데이트
                bars.attr('x', (d, i) => newXScale(i))
                    .attr('width', Math.max(1, this.width / this.frameData.length * event.transform.k));
            });

        svg.call(zoom);

        // Duration regions 표시 영역
        const regionsDiv = document.createElement('div');
        regionsDiv.id = `${this.containerId}-duration-regions`;
        regionsDiv.className = 'duration-regions-list';
        durationDiv.appendChild(regionsDiv);
    }

    /**
     * Duration regions 목록 업데이트
     */
    updateDurationRegionsDisplay() {
        const regionsDiv = document.getElementById(`${this.containerId}-duration-regions`);
        if (!regionsDiv) return;

        regionsDiv.innerHTML = '<h5>Duration 편집 구간:</h5>';

        if (this.durationRegions.length === 0) {
            regionsDiv.innerHTML += '<p style="color: #666;">구간 없음</p>';
            return;
        }

        const list = document.createElement('ul');
        this.durationRegions.forEach((region, idx) => {
            const li = document.createElement('li');
            li.innerHTML = `
                Frame ${region.startFrame} ~ ${region.endFrame}: Ratio = ${region.ratio.toFixed(2)}
                <button onclick="window.removeRegion('${this.containerId}', ${idx})">삭제</button>
            `;
            list.appendChild(li);
        });
        regionsDiv.appendChild(list);
    }

    /**
     * Duration region 삭제
     */
    removeRegion(index) {
        this.durationRegions.splice(index, 1);
        this.updateDurationRegionsDisplay();
    }

    /**
     * Pitch를 반음 단위로 변환 (기준: A4 = 440Hz)
     */
    pitchToSemitone(frequency) {
        if (frequency <= 0) return 0;
        return 12 * Math.log2(frequency / 440.0);
    }

    /**
     * 프레임 데이터의 pitch를 smoothing하여 안정적인 곡선 생성
     */
    getSmoothedPitchData() {
        if (this.frameData.length === 0) return [];

        // 원본 pitch를 반음으로 변환
        const rawPitch = this.frameData.map(f => this.pitchToSemitone(f.pitch));

        // Gaussian smoothing 적용
        const windowSize = 9;  // 홀수여야 함 (좌우 대칭)
        const sigma = 2.0;

        // Gaussian 가중치 계산
        const weights = [];
        const halfWindow = Math.floor(windowSize / 2);
        let weightSum = 0;

        for (let i = -halfWindow; i <= halfWindow; i++) {
            const weight = Math.exp(-(i * i) / (2 * sigma * sigma));
            weights.push(weight);
            weightSum += weight;
        }

        // 가중치 정규화
        for (let i = 0; i < weights.length; i++) {
            weights[i] /= weightSum;
        }

        // Gaussian smoothing 적용
        const smoothed = [];
        for (let i = 0; i < rawPitch.length; i++) {
            let sum = 0;
            let totalWeight = 0;

            for (let j = -halfWindow; j <= halfWindow; j++) {
                const idx = i + j;
                if (idx >= 0 && idx < rawPitch.length && rawPitch[idx] !== 0) {
                    sum += rawPitch[idx] * weights[j + halfWindow];
                    totalWeight += weights[j + halfWindow];
                }
            }

            smoothed.push(totalWeight > 0 ? sum / totalWeight : rawPitch[i]);
        }

        return smoothed;
    }

    /**
     * 반음 값을 계이름으로 변환
     */
    semitoneToNoteName(semitone) {
        const noteNames = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
        const octave = Math.floor((semitone + 9) / 12) + 4;  // A4 = 440Hz 기준
        const noteIndex = Math.round((semitone + 9) % 12);
        return `${noteNames[noteIndex < 0 ? noteIndex + 12 : noteIndex]}${octave}`;
    }

    /**
     * Key points로부터 모든 프레임의 pitch shift를 선형 보간
     * (원본 파형의 떨림을 유지하면서 shift 값만 보간)
     */
    interpolateAllFrames() {
        if (this.keyPoints.length < 2) return;

        // 각 프레임의 pitchEdits를 key points로부터 선형 보간
        for (let i = 0; i < this.frameData.length; i++) {
            // 현재 프레임이 어느 key points 사이에 있는지 찾기
            let leftIdx = 0;
            let rightIdx = 1;

            for (let k = 0; k < this.keyPoints.length - 1; k++) {
                if (i >= this.keyPoints[k].frameIndex && i <= this.keyPoints[k + 1].frameIndex) {
                    leftIdx = k;
                    rightIdx = k + 1;
                    break;
                }
            }

            const leftFrame = this.keyPoints[leftIdx].frameIndex;
            const rightFrame = this.keyPoints[rightIdx].frameIndex;
            const leftShift = this.keyPoints[leftIdx].semitones;
            const rightShift = this.keyPoints[rightIdx].semitones;

            // 선형 보간 (shift 값만 부드럽게 변화)
            // 원본 파형의 미세한 떨림과 비브라토는 그대로 유지됨
            const t = (rightFrame - leftFrame) > 0 ? (i - leftFrame) / (rightFrame - leftFrame) : 0;
            this.pitchEdits[i].semitones = leftShift + (rightShift - leftShift) * t;
        }

        // 디버그 로그
        const nonZeroEdits = this.pitchEdits.filter(e => Math.abs(e.semitones) > 0.01);
        console.log(`[interpolateAllFrames] 보간 완료: ${nonZeroEdits.length}개 프레임에 pitch shift 적용`);
        if (nonZeroEdits.length > 0) {
            console.log(`  범위: ${nonZeroEdits[0].frameIndex} ~ ${nonZeroEdits[nonZeroEdits.length - 1].frameIndex}`);
            console.log(`  Shift 값 예시: ${nonZeroEdits[0].semitones.toFixed(2)}, ${nonZeroEdits[Math.floor(nonZeroEdits.length / 2)].semitones.toFixed(2)}, ${nonZeroEdits[nonZeroEdits.length - 1].semitones.toFixed(2)}`);
        }
    }

    /**
     * 특정 key point 드래그 시 주변 key points에 영향 적용 (Gaussian)
     */
    applyInfluence(draggedIdx, newSemitones) {
        const draggedFrameIdx = this.keyPoints[draggedIdx].frameIndex;
        const delta = newSemitones - this.keyPoints[draggedIdx].semitones;

        // 드래그한 포인트 업데이트
        this.keyPoints[draggedIdx].semitones = newSemitones;

        // 주변 key points에 영향 적용 (Gaussian falloff)
        for (let i = 0; i < this.keyPoints.length; i++) {
            if (i === draggedIdx) continue;

            const distance = Math.abs(this.keyPoints[i].frameIndex - draggedFrameIdx);
            if (distance > this.influenceRadius) continue;

            // Gaussian weight 계산
            const sigma = this.influenceRadius / 3;  // 3-sigma rule
            const weight = Math.exp(-(distance * distance) / (2 * sigma * sigma));

            // 영향 적용
            this.keyPoints[i].semitones += delta * weight;
        }

        // 모든 프레임 재보간
        this.interpolateAllFrames();
    }

    /**
     * 현재 편집 데이터 가져오기
     * 이제 key points만 반환 (C++에서 interpolation 수행)
     */
    getEdits() {
        return {
            keyPoints: this.keyPoints,  // Key points만 반환
            durationRegions: this.durationRegions
        };
    }

    /**
     * 편집 초기화
     */
    reset() {
        this.pitchEdits = this.frameData.map((f, i) => ({ frameIndex: i, semitones: 0 }));

        // Key points도 초기화
        this.keyPoints = [];
        this.keyPoints.push({ frameIndex: 0, semitones: 0 });
        for (let i = this.keyPointInterval; i < this.frameData.length; i += this.keyPointInterval) {
            this.keyPoints.push({ frameIndex: i, semitones: 0 });
        }
        if (this.keyPoints[this.keyPoints.length - 1].frameIndex !== this.frameData.length - 1) {
            this.keyPoints.push({ frameIndex: this.frameData.length - 1, semitones: 0 });
        }

        this.durationRegions = [];
        this.render();
    }
}

// 전역 함수로 region 삭제 지원
window.removeRegion = function(containerId, index) {
    const editor = window[`editor_${containerId}`];
    if (editor) {
        editor.removeRegion(index);
    }
};
