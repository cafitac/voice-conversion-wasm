/**
 * UnifiedEditor - Pitch와 Duration을 하나의 그래프에서 편집
 */
export class UnifiedEditor {
    constructor(chartId, module = null) {
        this.chartId = chartId;
        this.svg = null;
        this.width = 0;
        this.height = 0;
        this.pitchData = [];
        this.durationSegments = [];
        this.sampleRate = 48000;
        this.module = module;  // WebAssembly Module

        // 편집 상태
        this.editIndices = [];  // 편집된 pitchData 인덱스 배열 [10, 20, 35, ...]
        this.durationEdits = []; // [{start, end, ratio}]

        // 오디오 전체 길이 (초)
        this.audioTotalDuration = 0;

        // 콜백
        this.onPitchEdit = null;
        this.onDurationEdit = null;
    }

    /**
     * WebAssembly Module 설정
     */
    setModule(module) {
        this.module = module;
    }


    /**
     * 통합 그래프 렌더링 (Pitch + Duration)
     */
    render(pitchData, durationData) {
        this.pitchData = pitchData;
        this.durationSegments = durationData || [];

        // 오디오 전체 길이 저장
        if (pitchData && pitchData.length > 0) {
            this.audioTotalDuration = Math.max(...pitchData.map(d => d.time));
        }

        const container = d3.select(`#${this.chartId}`);
        container.selectAll('*').remove();

        const margin = { top: 40, right: 40, bottom: 60, left: 60 };
        this.width = container.node().offsetWidth - margin.left - margin.right;

        // 브라우저 높이의 40%를 사용 (최소 300px, 최대 500px)
        const viewportHeight = window.innerHeight;
        const dynamicHeight = Math.max(300, Math.min(500, viewportHeight * 0.4));
        this.height = dynamicHeight - margin.top - margin.bottom;

        this.svg = container.append('svg')
            .attr('width', this.width + margin.left + margin.right)
            .attr('height', this.height + margin.top + margin.bottom);

        const g = this.svg.append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        // 두 영역으로 분할: 위쪽 Pitch, 아래쪽 Duration
        const pitchHeight = this.height * 0.5;
        const durationHeight = this.height * 0.35;
        const gap = this.height * 0.15;

        // X축 스케일 (시간)
        const maxTime = d3.max(pitchData, d => d.time);
        const xScale = d3.scaleLinear()
            .domain([0, maxTime])
            .range([0, this.width]);

        // Pitch 영역 렌더링
        this.renderPitchArea(g, pitchData, xScale, 0, pitchHeight);

        // Duration 영역 렌더링
        this.renderDurationArea(g, durationData, xScale, pitchHeight + gap, durationHeight);

        // X축 (시간)
        const xAxis = d3.axisBottom(xScale)
            .ticks(10)
            .tickFormat(d => `${d.toFixed(2)}s`);

        g.append('g')
            .attr('class', 'x-axis')
            .attr('transform', `translate(0,${this.height})`)
            .call(xAxis)
            .selectAll('text')
            .style('fill', 'var(--text-secondary)')
            .style('font-size', '12px');

        g.selectAll('.x-axis path, .x-axis line')
            .style('stroke', 'var(--border-color)');
    }

    /**
     * Pitch 영역 렌더링
     */
    renderPitchArea(g, data, xScale, yOffset, height) {
        const pitchGroup = g.append('g')
            .attr('class', 'pitch-area')
            .attr('transform', `translate(0,${yOffset})`);

        // 제목
        pitchGroup.append('text')
            .attr('x', this.width / 2)
            .attr('y', -10)
            .attr('text-anchor', 'middle')
            .style('fill', 'var(--text-primary)')
            .style('font-size', '16px')
            .style('font-weight', '600')
            .text('🎵 Pitch (Hz)');

        // Y축 스케일 (Pitch)
        const pitchValues = data.map(d => d.frequency).filter(f => f > 0);
        const minPitch = d3.min(pitchValues) * 0.9;
        const maxPitch = d3.max(pitchValues) * 1.1;

        const yScale = d3.scaleLinear()
            .domain([minPitch, maxPitch])
            .range([height, 0]);

        // Y축
        const yAxis = d3.axisLeft(yScale)
            .ticks(8)
            .tickFormat(d => `${Math.round(d)} Hz`);

        pitchGroup.append('g')
            .attr('class', 'y-axis')
            .call(yAxis)
            .selectAll('text')
            .style('fill', 'var(--text-secondary)')
            .style('font-size', '11px');

        pitchGroup.selectAll('.y-axis path, .y-axis line')
            .style('stroke', 'var(--border-color)');

        // 첫 렌더링: 편집 포인트 초기화 (C++에서 생성)
        if (this.editIndices.length === 0) {
            if (this.module && this.module.generateEditPoints) {
                this.editIndices = this.module.generateEditPoints(
                    data,
                    5,     // frameInterval: 5프레임 단위
                    50.0,  // gradientThreshold: 50Hz 이상 급변 (자동 변곡점 감지)
                    0.3    // confidenceThreshold
                );
                console.log(`🎯 C++ generated ${this.editIndices.length} edit points (5-frame + inflection)`);
            } else {
                // Fallback: JS로 생성
                this.editIndices = this.findPeaks(data);
                console.log(`🎯 JS generated ${this.editIndices.length} edit points (fallback)`);
            }
        }

        const filteredData = data.filter(d => d.confidence > 0.3);

        // 배경 선 그래프 (원본)
        const line = d3.line()
            .x(d => xScale(d.time))
            .y(d => yScale(d.frequency > 0 ? d.frequency : minPitch))
            .curve(d3.curveMonotoneX);

        pitchGroup.append('path')
            .datum(filteredData)
            .attr('class', 'original-pitch-line')
            .attr('fill', 'none')
            .attr('stroke', 'var(--accent-start)')
            .attr('stroke-width', 2)
            .attr('opacity', 0.3)
            .attr('d', line);

        // 편집된 선 (pitchData 직접 사용 - C++에서 이미 수정됨)
        pitchGroup.append('path')
            .datum(filteredData)
            .attr('class', 'edited-pitch-line')
            .attr('fill', 'none')
            .attr('stroke', 'var(--accent-start)')
            .attr('stroke-width', 2.5)
            .attr('d', line);

        // 편집 포인트 원들 렌더링 (editIndices는 전체 data 기준)
        const circleData = this.editIndices
            .filter(idx => idx < data.length && data[idx].confidence > 0.3)
            .map(idx => ({
                index: idx,
                ...data[idx]
            }));

        const points = pitchGroup.selectAll('.pitch-point')
            .data(circleData)
            .enter()
            .append('circle')
            .attr('class', 'pitch-point')
            .attr('cx', d => xScale(d.time))
            .attr('cy', d => yScale(d.frequency))
            .attr('r', 5)
            .attr('fill', 'var(--accent-end)')
            .attr('stroke', '#fff')
            .attr('stroke-width', 2)
            .style('cursor', 'ns-resize')
            .call(this.createPitchDrag(xScale, yScale, height, filteredData, pitchGroup));

        // 호버 효과
        points.on('mouseover', function() {
            d3.select(this)
                .transition()
                .duration(150)
                .attr('r', 7);
        }).on('mouseout', function() {
            d3.select(this)
                .transition()
                .duration(150)
                .attr('r', 5);
        });
    }

    /**
     * 편집 포인트 생성
     * 1. 10프레임 단위 균등 배치
     * 2. 급격한 변화(변곡점) 감지해서 추가
     */
    findPeaks(data) {
        if (data.length === 0) return [];

        const editIndicesSet = new Set();
        const frameInterval = 10; // 10프레임마다 편집 포인트

        // 1. 10프레임 단위 기본 배치
        for (let i = 0; i < data.length; i += frameInterval) {
            if (data[i].confidence > 0.3) {
                editIndicesSet.add(i);
            }
        }

        // 2. 급격한 변화(변곡점) 감지 - gradient 기반
        const gradientThreshold = 50; // Hz/frame
        for (let i = 1; i < data.length - 1; i++) {
            if (data[i].confidence < 0.3) continue;

            const prevFreq = data[i - 1].frequency;
            const currFreq = data[i].frequency;
            const nextFreq = data[i + 1].frequency;

            // 앞뒤 gradient 계산
            const gradient1 = Math.abs(currFreq - prevFreq);
            const gradient2 = Math.abs(nextFreq - currFreq);

            // 급격한 변화 감지 (꺾이는 부분)
            if (gradient1 > gradientThreshold || gradient2 > gradientThreshold) {
                editIndicesSet.add(i);
                // 변곡점 전후도 추가 (더 정확한 보간)
                if (i > 0 && data[i - 1].confidence > 0.3) editIndicesSet.add(i - 1);
                if (i < data.length - 1 && data[i + 1].confidence > 0.3) editIndicesSet.add(i + 1);
            }
        }

        // 3. 마지막 포인트 추가 (경계 처리)
        const lastIdx = data.length - 1;
        if (data[lastIdx].confidence > 0.3) {
            editIndicesSet.add(lastIdx);
        }

        // Set을 배열로 변환 후 정렬
        const editIndices = Array.from(editIndicesSet).sort((a, b) => a - b);
        console.log(`📍 Created ${editIndices.length} edit points (${frameInterval}-frame + inflection points)`);

        return editIndices;
    }

    /**
     * Duration 영역 렌더링
     */
    renderDurationArea(g, segments, xScale, yOffset, height) {
        const durationGroup = g.append('g')
            .attr('class', 'duration-area')
            .attr('transform', `translate(0,${yOffset})`);

        // 제목
        durationGroup.append('text')
            .attr('x', this.width / 2)
            .attr('y', -10)
            .attr('text-anchor', 'middle')
            .style('fill', 'var(--text-primary)')
            .style('font-size', '16px')
            .style('font-weight', '600')
            .text('⏱️ Duration Ratio');

        // 배경
        durationGroup.append('rect')
            .attr('width', this.width)
            .attr('height', height)
            .attr('fill', 'rgba(99, 102, 241, 0.08)')
            .attr('stroke', 'rgba(99, 102, 241, 0.2)')
            .attr('stroke-width', 1)
            .attr('rx', 8);

        // 안내 텍스트 (편집이 없을 때만 표시)
        if (this.durationEdits.length === 0) {
            durationGroup.append('text')
                .attr('x', this.width / 2)
                .attr('y', height / 2)
                .attr('text-anchor', 'middle')
                .attr('dominant-baseline', 'middle')
                .style('fill', 'var(--text-muted)')
                .style('font-size', '14px')
                .style('font-style', 'italic')
                .text('👆 마우스로 구간을 드래그하여 Duration 편집 추가');
        }

        // 기본 ratio 1.0 라인
        durationGroup.append('line')
            .attr('x1', 0)
            .attr('x2', this.width)
            .attr('y1', height / 2)
            .attr('y2', height / 2)
            .attr('stroke', 'var(--border-color)')
            .attr('stroke-dasharray', '4,4')
            .attr('stroke-width', 1);

        // Ratio 레이블
        durationGroup.append('text')
            .attr('x', -10)
            .attr('y', height / 2)
            .attr('text-anchor', 'end')
            .attr('dominant-baseline', 'middle')
            .style('fill', 'var(--text-muted)')
            .style('font-size', '11px')
            .text('1.0x');

        // Duration 편집 세그먼트들
        if (this.durationEdits.length > 0) {
            this.renderDurationSegments(durationGroup, xScale, height);
        }

        // 브러시로 구간 선택
        const brush = d3.brushX()
            .extent([[0, 0], [this.width, height]])
            .on('end', (event) => this.onBrushEnd(event, xScale));

        durationGroup.append('g')
            .attr('class', 'brush')
            .call(brush);
    }

    /**
     * Duration 세그먼트 렌더링
     */
    renderDurationSegments(group, xScale, height) {
        const yScale = d3.scaleLinear()
            .domain([0.5, 2.0])
            .range([height, 0]);

        const segments = group.selectAll('.duration-segment')
            .data(this.durationEdits)
            .enter()
            .append('g')
            .attr('class', 'duration-segment');

        // 세그먼트 사각형
        segments.append('rect')
            .attr('x', d => xScale(d.start))
            .attr('y', d => yScale(d.ratio))
            .attr('width', d => xScale(d.end) - xScale(d.start))
            .attr('height', d => height - yScale(d.ratio))
            .attr('fill', 'var(--success)')
            .attr('fill-opacity', 0.3)
            .attr('stroke', 'var(--success)')
            .attr('stroke-width', 2)
            .attr('rx', 4);

        // Ratio 텍스트
        segments.append('text')
            .attr('x', d => (xScale(d.start) + xScale(d.end)) / 2)
            .attr('y', d => yScale(d.ratio) - 5)
            .attr('text-anchor', 'middle')
            .style('fill', 'var(--text-primary)')
            .style('font-size', '12px')
            .style('font-weight', '600')
            .text(d => `${d.ratio.toFixed(2)}x`);

        // 삭제 버튼
        const deleteBtn = segments.append('g')
            .attr('class', 'delete-btn')
            .attr('transform', d => `translate(${xScale(d.end) - 15}, ${yScale(d.ratio) - 15})`)
            .style('cursor', 'pointer')
            .on('click', (event, d) => {
                event.stopPropagation();
                this.removeDurationEdit(d);
            });

        deleteBtn.append('circle')
            .attr('r', 10)
            .attr('fill', 'var(--danger)')
            .attr('stroke', '#fff')
            .attr('stroke-width', 2);

        deleteBtn.append('text')
            .attr('text-anchor', 'middle')
            .attr('dominant-baseline', 'middle')
            .style('fill', '#fff')
            .style('font-size', '12px')
            .style('font-weight', 'bold')
            .text('×');
    }

    /**
     * Pitch 드래그 생성 (인덱스 기반)
     */
    createPitchDrag(xScale, yScale, height, filteredData, pitchGroup) {
        let draggedCircle = null;
        let draggedIndex = null;
        let originalPitchData = null;

        return d3.drag()
            .on('start', (event, d) => {
                // 드래그 시작: 원본 데이터 백업
                draggedCircle = d3.select(event.sourceEvent.target);
                draggedIndex = d.index;
                originalPitchData = JSON.parse(JSON.stringify(this.pitchData));
                console.log(`🎵 Drag start: index=${draggedIndex}`);
            })
            .on('drag', (event, d) => {
                // 드래그 중: 로컬 선형 보간으로 실시간 피드백
                const newY = Math.max(0, Math.min(height, event.y));
                const newFreq = yScale.invert(newY);

                // 점 위치 임시 업데이트
                if (draggedCircle) {
                    draggedCircle.attr('cy', newY);
                }

                // 인접 편집 포인트 찾기
                const sortedIndices = [...this.editIndices].sort((a, b) => a - b);
                const currentIdx = sortedIndices.indexOf(draggedIndex);
                const prevEditIdx = currentIdx > 0 ? sortedIndices[currentIdx - 1] : -1;
                const nextEditIdx = currentIdx < sortedIndices.length - 1 ? sortedIndices[currentIdx + 1] : this.pitchData.length;

                // 임시 데이터 생성 (선형 보간) - filteredData만 업데이트
                const tempData = filteredData.map(point => {
                    // filteredData의 각 point에 해당하는 원본 인덱스 찾기
                    const originalIdx = this.pitchData.findIndex(p =>
                        Math.abs(p.time - point.time) < 0.001 && Math.abs(p.frequency - point.frequency) < 0.1
                    );

                    if (originalIdx === draggedIndex) {
                        return { ...point, frequency: newFreq };
                    } else if (originalIdx > prevEditIdx && originalIdx < nextEditIdx) {
                        // 드래그된 포인트와 인접 포인트 사이만 보간
                        if (originalIdx > prevEditIdx && originalIdx < draggedIndex) {
                            // prevEdit ~ dragged 구간
                            const t = (originalIdx - prevEditIdx) / (draggedIndex - prevEditIdx);
                            const prevFreq = prevEditIdx >= 0 ? originalPitchData[prevEditIdx].frequency : point.frequency;
                            return { ...point, frequency: prevFreq + t * (newFreq - prevFreq) };
                        } else if (originalIdx > draggedIndex && originalIdx < nextEditIdx) {
                            // dragged ~ nextEdit 구간
                            const t = (originalIdx - draggedIndex) / (nextEditIdx - draggedIndex);
                            const nextFreq = nextEditIdx < this.pitchData.length ? originalPitchData[nextEditIdx].frequency : point.frequency;
                            return { ...point, frequency: newFreq + t * (nextFreq - newFreq) };
                        }
                    }
                    return point;
                });

                // 선 업데이트
                const line = d3.line()
                    .x(p => xScale(p.time))
                    .y(p => yScale(p.frequency))
                    .curve(d3.curveMonotoneX);

                pitchGroup.select('.edited-pitch-line')
                    .datum(tempData)
                    .attr('d', line);
            })
            .on('end', async (event, d) => {
                if (draggedIndex === null) return;

                const newY = Math.max(0, Math.min(height, event.y));
                const newFreq = yScale.invert(newY);

                console.log(`🎵 Drag end: index=${draggedIndex}, newFreq=${newFreq.toFixed(2)} Hz`);

                // pitchData 업데이트
                this.pitchData[draggedIndex].frequency = newFreq;

                // C++ processPitchData 호출
                if (this.module && this.module.processPitchData) {
                    try {
                        // 변경된 인덱스 + 전체 편집 포인트 전달
                        const result = this.module.processPitchData(
                            this.pitchData,
                            draggedIndex,      // 방금 변경된 인덱스 (1개)
                            this.editIndices,  // 모든 편집 포인트 (16개)
                            3.0                // gradientThreshold
                        );

                        console.log(`✅ C++ returned ${result.pitchData.length} points, ${result.editIndices.length} edit indices`);

                        // 결과로 업데이트
                        this.pitchData = result.pitchData;
                        this.editIndices = result.editIndices;

                        // 전체 그래프 다시 렌더링
                        this.render(this.pitchData, this.durationSegments);

                        if (this.onPitchEdit) {
                            this.onPitchEdit(draggedIndex, newFreq);
                        }
                    } catch (error) {
                        console.error('processPitchData failed:', error);
                        // 에러 발생 시 원본 데이터로 복구
                        this.pitchData = originalPitchData;
                        this.render(this.pitchData, this.durationSegments);
                    }
                } else {
                    // Fallback: 모듈 없으면 그냥 렌더링
                    this.render(this.pitchData, this.durationSegments);
                }

                draggedCircle = null;
                draggedIndex = null;
                originalPitchData = null;
            });
    }


    /**
     * 브러시 종료 핸들러
     */
    onBrushEnd(event, xScale) {
        if (!event.selection) return;

        const [x0, x1] = event.selection;
        const startTime = xScale.invert(x0);
        const endTime = xScale.invert(x1);

        // Ratio 입력 받기
        const ratio = prompt(`구간 ${startTime.toFixed(2)}s ~ ${endTime.toFixed(2)}s의 재생 속도 비율을 입력하세요 (0.5 ~ 2.0)`, '1.5');

        if (ratio === null) return;

        const ratioFloat = parseFloat(ratio);
        if (isNaN(ratioFloat) || ratioFloat < 0.5 || ratioFloat > 2.0) {
            alert('0.5 ~ 2.0 사이의 값을 입력하세요.');
            return;
        }

        // Duration 편집 추가
        this.addDurationEdit({
            start: startTime,
            end: endTime,
            ratio: ratioFloat
        });

        // 브러시 클리어
        d3.select(event.sourceEvent.target.parentNode).call(event.target.clear);

        // 다시 렌더링
        this.render(this.pitchData, this.durationSegments);

        if (this.onDurationEdit) {
            this.onDurationEdit(this.durationEdits);
        }
    }

    /**
     * Duration 편집 추가
     */
    addDurationEdit(edit) {
        this.durationEdits.push(edit);
        this.durationEdits.sort((a, b) => a.start - b.start);
    }

    /**
     * Duration 편집 제거
     */
    removeDurationEdit(editToRemove) {
        this.durationEdits = this.durationEdits.filter(e => e !== editToRemove);
        this.render(this.pitchData, this.durationSegments);

        if (this.onDurationEdit) {
            this.onDurationEdit(this.durationEdits);
        }
    }

    /**
     * 모든 편집 가져오기
     */
    getEdits() {
        return {
            pitch: this.editIndices.map(idx => ({
                index: idx,
                time: this.pitchData[idx]?.time,
                frequency: this.pitchData[idx]?.frequency
            })),
            duration: this.durationEdits,
            pitchData: this.pitchData  // C++ 처리된 전체 pitchData
        };
    }

    /**
     * 편집 초기화
     */
    reset() {
        this.editIndices = [];
        this.durationEdits = [];

        if (this.pitchData.length > 0) {
            this.render(this.pitchData, this.durationSegments);
        }
    }
}
