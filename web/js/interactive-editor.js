/**
 * InteractiveEditor - D3.js 기반 Pitch/Duration 분석 표시
 *
 * 편집 기능 없이 분석 결과만 시각화합니다.
 * - Pitch: 시간에 따른 음높이 변화 (계이름 표시)
 * - Duration: RMS 기반 음성 구간 표시
 * - Zoom/Pan 지원
 */
export class InteractiveEditor {
    constructor(containerId) {
        this.containerId = containerId;
        this.container = document.getElementById(containerId);
        this.frameData = [];

        this.margin = { top: 20, right: 30, bottom: 50, left: 80 };
        this.width = 0;
        this.height = 0;

        this.svg = null;
        this.pitchChart = null;
        this.durationChart = null;
    }

    /**
     * 프레임 데이터 설정 및 차트 생성
     */
    setFrameData(frames) {
        this.frameData = frames;
        console.log(`[InteractiveEditor] 프레임 데이터 설정: ${frames.length}개 프레임`);
        this.render();
    }

    /**
     * 전체 차트 렌더링
     */
    render() {
        console.log('[InteractiveEditor] render() 시작', {
            container: !!this.container,
            frameDataLength: this.frameData.length
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
        this.height = 400 - this.margin.top - this.margin.bottom;

        console.log('[InteractiveEditor] 차트 크기:', { width: this.width, height: this.height });

        // Pitch 차트 생성
        this.renderPitchChart();

        // Duration 차트 생성
        this.renderDurationChart();

        console.log('[InteractiveEditor] render() 완료');
    }

    /**
     * Pitch 차트 렌더링 (곡선 + Zoom/Pan)
     */
    renderPitchChart() {
        console.log('[InteractiveEditor] renderPitchChart() 시작');

        const pitchDiv = document.createElement('div');
        pitchDiv.className = 'chart-section';
        this.container.appendChild(pitchDiv);

        const title = document.createElement('h4');
        title.textContent = 'Pitch 분석 (마우스 휠로 확대/축소)';
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

        // Y축: 반음 범위 계산
        const validPitch = smoothedPitch.filter(p => p !== 0);
        const minPitch = validPitch.length > 0 ? Math.min(...validPitch) : -12;
        const maxPitch = validPitch.length > 0 ? Math.max(...validPitch) : 12;

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

        // Y축: 계이름으로 표시
        const yAxisTicks = [];
        for (let s = Math.floor(yMin); s <= Math.ceil(yMax); s += 2) {
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

        // Pitch 라인 (파란색, smoothed & 부드러운 곡선)
        const pitchLine = d3.line()
            .defined((d, i) => smoothedPitch[i] !== 0)
            .x((d, i) => xScale(i))
            .y((d, i) => yScale(smoothedPitch[i]))
            .curve(d3.curveBasis);

        const pitchPath = zoomGroup.append('path')
            .datum(this.frameData)
            .attr('fill', 'none')
            .attr('stroke', '#3498db')
            .attr('stroke-width', 2.5)
            .attr('d', pitchLine);

        // Zoom 동작
        const zoom = d3.zoom()
            .scaleExtent([1, 50])
            .translateExtent([[0, 0], [this.width, this.height]])
            .extent([[0, 0], [this.width, this.height]])
            .on('zoom', (event) => {
                const newXScale = event.transform.rescaleX(xScale);

                xAxisGroup.call(d3.axisBottom(newXScale).ticks(10));

                pitchPath.attr('d', pitchLine.x((d, i) => newXScale(i)));

                if (zeroLine) {
                    zeroLine.attr('x2', this.width * event.transform.k);
                }
            });

        svg.call(zoom);

        console.log('[InteractiveEditor] Pitch 차트 렌더링 완료');
    }

    /**
     * Duration 차트 렌더링 (RMS 막대 + Zoom/Pan)
     */
    renderDurationChart() {
        const durationDiv = document.createElement('div');
        durationDiv.className = 'chart-section';
        this.container.appendChild(durationDiv);

        const title = document.createElement('h4');
        title.textContent = 'Duration 분석 (초록: 음성 구간 | 회색: 무음)';
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

        // Y축
        g.append('g')
            .call(d3.axisLeft(yScale).ticks(5).tickFormat(d => (d * 100).toFixed(0) + '%'))
            .append('text')
            .attr('transform', 'rotate(-90)')
            .attr('x', -this.height / 2)
            .attr('y', -50)
            .attr('fill', 'currentColor')
            .style('text-anchor', 'middle')
            .text('RMS (음성 에너지)');

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
            .attr('opacity', 0.7);

        // Zoom 동작
        const zoom = d3.zoom()
            .scaleExtent([1, 50])
            .translateExtent([[0, 0], [this.width, this.height]])
            .extent([[0, 0], [this.width, this.height]])
            .on('zoom', (event) => {
                const newXScale = event.transform.rescaleX(xScale);

                // X축 업데이트
                xAxisGroup.call(d3.axisBottom(newXScale).ticks(10));

                // 막대 위치 및 너비 업데이트
                bars.attr('x', (d, i) => newXScale(i))
                    .attr('width', Math.max(1, this.width / this.frameData.length * event.transform.k));
            });

        svg.call(zoom);

        console.log('[InteractiveEditor] Duration 차트 렌더링 완료');
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
        const windowSize = 9;
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
        const octave = Math.floor((semitone + 9) / 12) + 4;
        const noteIndex = Math.round((semitone + 9) % 12);
        return `${noteNames[noteIndex < 0 ? noteIndex + 12 : noteIndex]}${octave}`;
    }

    /**
     * 편집 데이터 가져오기 (호환성 유지용 - 빈 데이터 반환)
     */
    getEdits() {
        return {
            keyPoints: [],
            durationRegions: []
        };
    }

    /**
     * 초기화 (호환성 유지용)
     */
    reset() {
        this.render();
    }
}
