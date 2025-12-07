/**
 * PerformanceReport - Sentry-style 성능 보고서 시각화
 * C++/JS 성능 비교 및 계층적 트레이스 표시
 */
export class PerformanceReport {
    constructor() {
        this.cppReport = null;
        this.jsReport = null;
    }

    /**
     * C++ 보고서 설정 (세션 저장은 별도 호출)
     */
    setCppReport(report) {
        this.cppReport = report;
    }

    /**
     * JavaScript 보고서 설정 (세션 저장은 별도 호출)
     */
    setJsReport(report) {
        this.jsReport = report;
    }

    /**
     * 세션 보고서 저장 (C++ + JS + 설정)
     */
    saveSessionReport(cppReport, jsReport, settings) {
        try {
            const timestamp = new Date().toISOString();
            const id = `session_${Date.now()}`;

            const session = {
                id,
                timestamp,
                settings: settings || {},
                cpp: cppReport,
                js: jsReport
            };

            // 기존 세션 목록 가져오기
            let sessions = this.getSessions();

            // 새 세션 추가 (최신이 앞에)
            sessions.unshift(session);

            // 최근 10개만 유지
            if (sessions.length > 10) {
                sessions = sessions.slice(0, 10);
            }

            // 저장
            localStorage.setItem('performanceSessions', JSON.stringify(sessions));

            return session;
        } catch (error) {
            console.error('Failed to save session:', error);
            return null;
        }
    }

    /**
     * 모든 세션 가져오기
     */
    getSessions() {
        try {
            const data = localStorage.getItem('performanceSessions');
            return data ? JSON.parse(data) : [];
        } catch (error) {
            console.error('Failed to load sessions:', error);
            return [];
        }
    }

    /**
     * 특정 세션 로드
     */
    loadSession(sessionId) {
        try {
            const sessions = this.getSessions();
            const session = sessions.find(s => s.id === sessionId);

            if (session) {
                this.cppReport = session.cpp;
                this.jsReport = session.js;
                return session;
            }

            return null;
        } catch (error) {
            console.error('Failed to load session:', error);
            return null;
        }
    }

    /**
     * LocalStorage에서 최신 세션 로드
     */
    loadFromLocalStorage() {
        try {
            const sessions = this.getSessions();

            if (sessions.length > 0) {
                const latestSession = sessions[0];
                this.cppReport = latestSession.cpp;
                this.jsReport = latestSession.js;
            }
        } catch (error) {
            console.error('Failed to load reports:', error);
        }
    }

    /**
     * 세션 삭제
     */
    deleteSession(sessionId) {
        try {
            let sessions = this.getSessions();
            sessions = sessions.filter(s => s.id !== sessionId);
            localStorage.setItem('performanceSessions', JSON.stringify(sessions));
            return true;
        } catch (error) {
            console.error('Failed to delete session:', error);
            return false;
        }
    }

    /**
     * 선택된 세션들 삭제
     */
    deleteSelectedSessions(sessionIds) {
        try {
            let sessions = this.getSessions();
            sessions = sessions.filter(s => !sessionIds.includes(s.id));
            localStorage.setItem('performanceSessions', JSON.stringify(sessions));
            return true;
        } catch (error) {
            console.error('Failed to delete sessions:', error);
            return false;
        }
    }

    /**
     * 비교 보고서 렌더링
     */
    renderComparison() {
        if (!this.cppReport && !this.jsReport) {
            document.getElementById('comparisonTab').innerHTML = `
                <div class="empty-state">
                    <p>보고서가 없습니다</p>
                    <p class="hint">음성 변환을 실행하여 성능 보고서를 생성하세요</p>
                </div>
            `;
            return;
        }

        // 비교 테이블 생성
        this.renderComparisonTable();

        // 비교 차트 생성
        this.renderComparisonChart();
    }

    /**
     * 비교 테이블 렌더링
     */
    renderComparisonTable() {
        const tbody = document.querySelector('#comparisonTable tbody');
        if (!tbody) {
            console.warn('Comparison table tbody not found');
            return;
        }
        tbody.innerHTML = '';

        if (!this.cppReport || !this.jsReport) {
            tbody.innerHTML = `
                <tr>
                    <td colspan="4" style="text-align: center; color: var(--text-muted);">
                        두 언어 모두 실행해야 비교할 수 있습니다
                    </td>
                </tr>
            `;
            return;
        }

        // 기능별로 비교
        const features = ['total', 'pitch', 'duration', 'filter', 'reverse'];
        const featureNames = {
            'total': '전체 변환',
            'pitch': 'Pitch 조절',
            'duration': 'Duration 조절',
            'filter': '필터',
            'reverse': '역재생'
        };

        features.forEach(feature => {
            const cppTime = this.getCppFeatureTime(feature);
            const jsTime = this.getJsFeatureTime(feature);

            if (cppTime === null && jsTime === null) return;

            const speedup = jsTime / cppTime;
            const speedupClass = speedup > 1 ? 'speedup-faster' : 'speedup-slower';

            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${featureNames[feature]}</td>
                <td>${cppTime !== null ? cppTime.toFixed(2) : 'N/A'}</td>
                <td>${jsTime !== null ? jsTime.toFixed(2) : 'N/A'}</td>
                <td class="${speedupClass}">${speedup.toFixed(2)}x</td>
            `;
            tbody.appendChild(row);
        });
    }

    /**
     * C++ 기능 실행 시간 가져오기
     */
    getCppFeatureTime(feature) {
        if (!this.cppReport || !this.cppReport.features) return null;

        // 특별 케이스: 'total'은 totalDuration 사용
        if (feature === 'total') {
            return this.cppReport.totalDuration || null;
        }

        const featureNode = this.cppReport.features.find(f =>
            f.feature.toLowerCase().includes(feature.toLowerCase())
        );

        return featureNode ? featureNode.duration : null;
    }

    /**
     * JS 기능 실행 시간 가져오기
     */
    getJsFeatureTime(feature) {
        if (!this.jsReport || !this.jsReport.features) return null;

        // 특별 케이스: 'total'은 totalDuration 사용
        if (feature === 'total') {
            return this.jsReport.totalDuration || null;
        }

        // Feature 검색
        const featureNode = this.jsReport.features.find(f =>
            f.feature.toLowerCase().includes(feature.toLowerCase())
        );

        return featureNode ? featureNode.duration : null;
    }

    /**
     * 비교 차트 렌더링 (D3.js)
     */
    renderComparisonChart() {
        const container = d3.select('#comparisonChart');
        container.selectAll('*').remove();

        if (!this.cppReport || !this.jsReport) return;

        const data = [
            { feature: '전체 변환', cpp: this.getCppFeatureTime('total'), js: this.getJsFeatureTime('total') },
            { feature: 'Pitch', cpp: this.getCppFeatureTime('pitch'), js: this.getJsFeatureTime('pitch') },
            { feature: 'Duration', cpp: this.getCppFeatureTime('duration'), js: this.getJsFeatureTime('duration') },
            { feature: 'Filter', cpp: this.getCppFeatureTime('filter'), js: this.getJsFeatureTime('filter') },
            { feature: 'Reverse', cpp: this.getCppFeatureTime('reverse'), js: this.getJsFeatureTime('reverse') }
        ].filter(d => d.cpp !== null || d.js !== null);

        const margin = { top: 20, right: 80, bottom: 40, left: 60 };
        const width = container.node().offsetWidth - margin.left - margin.right;
        const height = 250 - margin.top - margin.bottom;

        const svg = container.append('svg')
            .attr('width', width + margin.left + margin.right)
            .attr('height', height + margin.top + margin.bottom)
            .append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        // X scale
        const x0 = d3.scaleBand()
            .domain(data.map(d => d.feature))
            .range([0, width])
            .padding(0.2);

        const x1 = d3.scaleBand()
            .domain(['cpp', 'js'])
            .range([0, x0.bandwidth()])
            .padding(0.05);

        // Y scale
        const maxValue = d3.max(data, d => Math.max(d.cpp || 0, d.js || 0));
        const y = d3.scaleLinear()
            .domain([0, maxValue * 1.1])
            .range([height, 0]);

        // Color scale
        const color = d3.scaleOrdinal()
            .domain(['cpp', 'js'])
            .range(['#667eea', '#4caf50']);

        // X axis
        svg.append('g')
            .attr('transform', `translate(0,${height})`)
            .call(d3.axisBottom(x0))
            .selectAll('text')
            .style('fill', 'var(--text-secondary)')
            .style('font-size', '10px');

        // Y axis
        svg.append('g')
            .call(d3.axisLeft(y).ticks(5).tickFormat(d => `${d}ms`))
            .selectAll('text')
            .style('fill', 'var(--text-secondary)')
            .style('font-size', '10px');

        // Bars
        const feature = svg.selectAll('.feature')
            .data(data)
            .enter().append('g')
            .attr('transform', d => `translate(${x0(d.feature)},0)`);

        feature.selectAll('rect')
            .data(d => ['cpp', 'js'].map(key => ({ key, value: d[key], feature: d.feature })))
            .enter().append('rect')
            .attr('x', d => x1(d.key))
            .attr('y', d => y(d.value || 0))
            .attr('width', x1.bandwidth())
            .attr('height', d => height - y(d.value || 0))
            .attr('fill', d => color(d.key))
            .attr('opacity', 0.8);

        // Legend
        const legend = svg.append('g')
            .attr('transform', `translate(${width - 60}, 0)`);

        ['cpp', 'js'].forEach((key, i) => {
            const g = legend.append('g')
                .attr('transform', `translate(0, ${i * 20})`);

            g.append('rect')
                .attr('width', 12)
                .attr('height', 12)
                .attr('fill', color(key))
                .attr('opacity', 0.8);

            g.append('text')
                .attr('x', 18)
                .attr('y', 10)
                .text(key === 'cpp' ? 'C++' : 'JavaScript')
                .style('fill', 'var(--text-primary)')
                .style('font-size', '10px');
        });
    }

    /**
     * C++ 상세 보고서 렌더링 (Sentry-style Trace)
     */
    renderCppDetails() {
        const container = document.getElementById('cppDetails');

        if (!this.cppReport || !this.cppReport.features) {
            container.innerHTML = `
                <div class="empty-state">
                    <p>C++ 보고서가 없습니다</p>
                </div>
            `;
            return;
        }

        container.innerHTML = '';

        // 성능 요약 정보
        const summary = document.createElement('div');
        summary.className = 'performance-summary';
        summary.innerHTML = `
            <h3>성능 요약</h3>
            <div class="summary-grid">
                <div class="summary-item">
                    <span class="summary-label">전체 실행 시간</span>
                    <span class="summary-value">${this.cppReport.totalDuration.toFixed(2)}ms</span>
                </div>
                <div class="summary-item">
                    <span class="summary-label">측정된 기능</span>
                    <span class="summary-value">${this.cppReport.features.length}개</span>
                </div>
                <div class="summary-item">
                    <span class="summary-label">엔진</span>
                    <span class="summary-value">C++ (WASM)</span>
                </div>
            </div>
        `;
        container.appendChild(summary);

        // 트레이스 타임라인
        const timelineHeader = document.createElement('h3');
        timelineHeader.textContent = '트레이스 타임라인';
        timelineHeader.style.marginTop = '24px';
        container.appendChild(timelineHeader);

        const timeline = document.createElement('div');
        timeline.className = 'trace-timeline';

        const totalDuration = this.cppReport.totalDuration || 1;

        this.cppReport.features.forEach(feature => {
            this.renderTraceItem(timeline, feature, 0, totalDuration, false);
        });

        container.appendChild(timeline);
    }

    /**
     * 트레이스 아이템 렌더링 (재귀적, 접기/펼치기 지원)
     */
    renderTraceItem(container, node, level, totalDuration, isExpanded = false) {
        const hasChildren = node.functions && node.functions.length > 0;

        // 아이템 컨테이너
        const itemWrapper = document.createElement('div');
        itemWrapper.className = 'trace-item-wrapper';

        const item = document.createElement('div');
        item.className = `trace-item ${level > 0 ? `level-${level}` : ''} ${hasChildren ? 'has-children' : ''}`;

        const percent = (node.duration / totalDuration * 100).toFixed(1);
        const barWidth = Math.max(2, percent);

        // 접기/펼치기 아이콘 추가
        const expandIcon = hasChildren ? `<span class="expand-icon ${isExpanded ? 'expanded' : ''}">${isExpanded ? '▼' : '▶'}</span>` : '<span class="expand-icon-placeholder"></span>';

        item.innerHTML = `
            ${expandIcon}
            <div class="trace-item-bar" style="width: ${barWidth}%"></div>
            <span class="trace-item-label">${node.feature || node.name}</span>
            <span class="trace-item-duration">${node.duration.toFixed(2)}ms</span>
            <span class="trace-item-percent">${percent}%</span>
        `;

        itemWrapper.appendChild(item);

        // 자식 함수들을 담을 컨테이너
        const childrenContainer = document.createElement('div');
        childrenContainer.className = 'trace-children';
        childrenContainer.style.display = isExpanded ? 'block' : 'none';

        // 자식 함수들 렌더링
        if (hasChildren) {
            node.functions.forEach(func => {
                this.renderFunctionItem(childrenContainer, func, level + 1, totalDuration, false);
            });
            itemWrapper.appendChild(childrenContainer);

            // 클릭 이벤트로 접기/펼치기
            item.style.cursor = 'pointer';
            item.addEventListener('click', (e) => {
                e.stopPropagation();
                const icon = item.querySelector('.expand-icon');
                const isCurrentlyExpanded = childrenContainer.style.display === 'block';

                childrenContainer.style.display = isCurrentlyExpanded ? 'none' : 'block';
                icon.textContent = isCurrentlyExpanded ? '▶' : '▼';
                icon.classList.toggle('expanded');
            });
        }

        container.appendChild(itemWrapper);
    }

    /**
     * 함수 아이템 렌더링 (재귀적, 접기/펼치기 지원)
     */
    renderFunctionItem(container, func, level, totalDuration, isExpanded = false) {
        const hasChildren = func.children && func.children.length > 0;

        // 아이템 컨테이너
        const itemWrapper = document.createElement('div');
        itemWrapper.className = 'trace-item-wrapper';

        const item = document.createElement('div');
        item.className = `trace-item level-${level} ${hasChildren ? 'has-children' : ''}`;

        const percent = (func.duration / totalDuration * 100).toFixed(1);
        const barWidth = Math.max(2, percent);

        // 접기/펼치기 아이콘 추가
        const expandIcon = hasChildren ? `<span class="expand-icon ${isExpanded ? 'expanded' : ''}">${isExpanded ? '▼' : '▶'}</span>` : '<span class="expand-icon-placeholder"></span>';

        item.innerHTML = `
            ${expandIcon}
            <div class="trace-item-bar" style="width: ${barWidth}%"></div>
            <span class="trace-item-label">${func.name}</span>
            <span class="trace-item-duration">${func.duration.toFixed(2)}ms</span>
            <span class="trace-item-percent">${percent}%</span>
        `;

        itemWrapper.appendChild(item);

        // 자식 함수들을 담을 컨테이너
        const childrenContainer = document.createElement('div');
        childrenContainer.className = 'trace-children';
        childrenContainer.style.display = isExpanded ? 'block' : 'none';

        // 자식 함수들 렌더링
        if (hasChildren) {
            func.children.forEach(child => {
                this.renderFunctionItem(childrenContainer, child, level + 1, totalDuration, false);
            });
            itemWrapper.appendChild(childrenContainer);

            // 클릭 이벤트로 접기/펼치기
            item.style.cursor = 'pointer';
            item.addEventListener('click', (e) => {
                e.stopPropagation();
                const icon = item.querySelector('.expand-icon');
                const isCurrentlyExpanded = childrenContainer.style.display === 'block';

                childrenContainer.style.display = isCurrentlyExpanded ? 'none' : 'block';
                icon.textContent = isCurrentlyExpanded ? '▶' : '▼';
                icon.classList.toggle('expanded');
            });
        }

        container.appendChild(itemWrapper);
    }

    /**
     * JavaScript 상세 보고서 렌더링 (C++과 동일한 계층적 구조)
     */
    renderJsDetails() {
        const container = document.getElementById('jsDetails');

        if (!this.jsReport || !this.jsReport.features) {
            container.innerHTML = `
                <div class="empty-state">
                    <p>JavaScript 보고서가 없습니다</p>
                </div>
            `;
            return;
        }

        container.innerHTML = '';

        // 성능 요약 정보
        const summary = document.createElement('div');
        summary.className = 'performance-summary';
        summary.innerHTML = `
            <h3>성능 요약</h3>
            <div class="summary-grid">
                <div class="summary-item">
                    <span class="summary-label">전체 실행 시간</span>
                    <span class="summary-value">${this.jsReport.totalDuration.toFixed(2)}ms</span>
                </div>
                <div class="summary-item">
                    <span class="summary-label">측정된 기능</span>
                    <span class="summary-value">${this.jsReport.features.length}개</span>
                </div>
                <div class="summary-item">
                    <span class="summary-label">엔진</span>
                    <span class="summary-value">JavaScript</span>
                </div>
            </div>
        `;
        container.appendChild(summary);

        // 트레이스 타임라인
        const timelineHeader = document.createElement('h3');
        timelineHeader.textContent = '트레이스 타임라인';
        timelineHeader.style.marginTop = '24px';
        container.appendChild(timelineHeader);

        const timeline = document.createElement('div');
        timeline.className = 'trace-timeline';

        const totalDuration = this.jsReport.totalDuration || 1;

        this.jsReport.features.forEach(feature => {
            this.renderTraceItem(timeline, feature, 0, totalDuration, false);
        });

        container.appendChild(timeline);
    }

    /**
     * JSON 다운로드
     */
    exportJSON() {
        const data = {
            cpp: this.cppReport,
            js: this.jsReport,
            timestamp: Date.now()
        };

        const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = `performance_report_${Date.now()}.json`;
        a.click();

        URL.revokeObjectURL(url);
    }

    /**
     * CSV 다운로드
     */
    exportCSV() {
        let csv = 'Feature,C++ (ms),JavaScript (ms),Speedup\n';

        const features = ['total', 'pitch', 'duration', 'filter', 'reverse'];
        const featureNames = {
            'total': '전체 변환',
            'pitch': 'Pitch 조절',
            'duration': 'Duration 조절',
            'filter': '필터',
            'reverse': '역재생'
        };

        features.forEach(feature => {
            const cppTime = this.getCppFeatureTime(feature);
            const jsTime = this.getJsFeatureTime(feature);

            if (cppTime === null && jsTime === null) return;

            const speedup = jsTime / cppTime;

            csv += `${featureNames[feature]},`;
            csv += `${cppTime !== null ? cppTime.toFixed(2) : 'N/A'},`;
            csv += `${jsTime !== null ? jsTime.toFixed(2) : 'N/A'},`;
            csv += `${speedup.toFixed(2)}x\n`;
        });

        const blob = new Blob([csv], { type: 'text/csv' });
        const url = URL.createObjectURL(blob);

        const a = document.createElement('a');
        a.href = url;
        a.download = `performance_report_${Date.now()}.csv`;
        a.click();

        URL.revokeObjectURL(url);
    }

    /**
     * 모든 보고서 초기화
     */
    clearAll() {
        if (confirm('모든 성능 보고서를 삭제하시겠습니까?')) {
            try {
                localStorage.removeItem('performanceSessions');

                this.cppReport = null;
                this.jsReport = null;

                alert('모든 보고서가 삭제되었습니다.');
                this.renderComparison();
                this.renderCppDetails();
                this.renderJsDetails();
                this.renderReportList(); // 목록도 업데이트
            } catch (error) {
                console.error('Failed to clear reports:', error);
                alert('보고서 삭제 실패');
            }
        }
    }

    /**
     * 보고서 목록 렌더링
     */
    renderReportList() {
        const container = document.getElementById('reportList');
        if (!container) return;

        const sessions = this.getSessions();

        if (sessions.length === 0) {
            container.innerHTML = `
                <div class="report-list-empty">
                    <p>저장된 보고서가 없습니다</p>
                    <p class="hint">변환 버튼을 눌러 보고서를 생성하세요</p>
                </div>
            `;
            return;
        }

        container.innerHTML = sessions.map(session => {
            const timestamp = new Date(session.timestamp).toLocaleString('ko-KR');
            const settings = this.formatSettings(session.settings);
            const cppTime = session.cpp ? session.cpp.totalDuration.toFixed(1) : 'N/A';
            const jsTime = session.js ? session.js.totalDuration.toFixed(1) : 'N/A';

            // C++ 기준으로 성능 비율 계산
            let ratioText = 'N/A';
            if (session.cpp && session.js) {
                const ratio = session.js.totalDuration / session.cpp.totalDuration;
                const percentDiff = ((ratio - 1) * 100).toFixed(1);
                const timeDiff = Math.abs(session.js.totalDuration - session.cpp.totalDuration).toFixed(1);

                if (ratio >= 1.0) {
                    // JS가 더 느림 = C++가 더 빠름
                    ratioText = `C++ ${timeDiff}ms 빠름 (${percentDiff}%)`;
                } else {
                    // JS가 더 빠름 (거의 없겠지만)
                    ratioText = `JS ${timeDiff}ms 빠름 (${Math.abs(percentDiff).toFixed(1)}%)`;
                }
            }

            return `
                <div class="report-item" data-session-id="${session.id}">
                    <div class="report-item-checkbox">
                        <input type="checkbox" class="report-checkbox" value="${session.id}">
                    </div>
                    <div class="report-item-info">
                        <div class="report-item-timestamp">${timestamp}</div>
                        <div class="report-item-settings">${settings}</div>
                    </div>
                    <div class="report-item-performance">
                        <div class="report-item-time">C++: ${cppTime}ms / JS: ${jsTime}ms</div>
                        <div class="report-item-ratio">${ratioText}</div>
                    </div>
                </div>
            `;
        }).join('');

        // 이벤트 핸들러 설정
        this.setupReportListHandlers();
    }

    /**
     * 설정 정보를 문자열로 포맷
     */
    formatSettings(settings) {
        const parts = [];

        if (settings.pitch && settings.pitch !== 0) {
            parts.push(`피치: ${settings.pitch > 0 ? '+' : ''}${settings.pitch}`);
        }

        if (settings.timeStretch && settings.timeStretch !== 1.0) {
            parts.push(`속도: ${settings.timeStretch}x`);
        }

        if (settings.filter && settings.filter !== 'none') {
            const filterNames = {
                '0': '컵 속 목소리',
                '1': '무전기',
                '3': '로봇 목소리',
                '4': '메아리',
                '5': '잔향',
                '6': '기타 앰프',
                '7': 'AM 라디오',
                '8': '합창 효과',
                '9': '플랜저',
                '10': '뉴스 인터뷰',
                '11': '범인 목소리'
            };
            parts.push(`필터: ${filterNames[settings.filter] || settings.filter}`);
        }

        if (settings.reverse) {
            parts.push('역재생');
        }

        return parts.length > 0 ? parts.join(', ') : '효과 없음';
    }

    /**
     * 보고서 목록 이벤트 핸들러 설정
     */
    setupReportListHandlers() {
        // 전체 선택 체크박스
        const selectAllCheckbox = document.getElementById('selectAllReports');
        if (selectAllCheckbox) {
            selectAllCheckbox.addEventListener('change', (e) => {
                const checkboxes = document.querySelectorAll('.report-checkbox');
                checkboxes.forEach(cb => cb.checked = e.target.checked);
                this.updateDeleteButtonState();
            });
        }

        // 개별 체크박스
        const checkboxes = document.querySelectorAll('.report-checkbox');
        checkboxes.forEach(cb => {
            cb.addEventListener('change', () => {
                this.updateDeleteButtonState();
            });
        });

        // 보고서 항목 클릭 (상세 보기)
        const reportItems = document.querySelectorAll('.report-item');
        reportItems.forEach(item => {
            item.addEventListener('click', (e) => {
                // 체크박스 클릭은 무시
                if (e.target.type === 'checkbox') return;

                const sessionId = item.dataset.sessionId;
                this.showReportDetail(sessionId);
            });
        });

        // 선택 삭제 버튼
        const deleteBtn = document.getElementById('deleteSelectedReports');
        if (deleteBtn) {
            deleteBtn.addEventListener('click', () => {
                this.deleteSelectedReportsHandler();
            });
        }
    }

    /**
     * 삭제 버튼 상태 업데이트
     */
    updateDeleteButtonState() {
        const checkboxes = document.querySelectorAll('.report-checkbox:checked');
        const deleteBtn = document.getElementById('deleteSelectedReports');
        if (deleteBtn) {
            deleteBtn.disabled = checkboxes.length === 0;
        }
    }

    /**
     * 선택된 보고서 삭제 핸들러
     */
    deleteSelectedReportsHandler() {
        const checkboxes = document.querySelectorAll('.report-checkbox:checked');
        const sessionIds = Array.from(checkboxes).map(cb => cb.value);

        if (sessionIds.length === 0) return;

        const confirmMsg = `선택한 ${sessionIds.length}개의 보고서를 삭제하시겠습니까?`;
        if (!confirm(confirmMsg)) return;

        const success = this.deleteSelectedSessions(sessionIds);

        if (success) {
            alert('선택한 보고서가 삭제되었습니다.');
            this.renderReportList();

            // 전체 선택 체크박스 해제
            const selectAllCheckbox = document.getElementById('selectAllReports');
            if (selectAllCheckbox) {
                selectAllCheckbox.checked = false;
            }
        } else {
            alert('보고서 삭제 실패');
        }
    }

    /**
     * 상세 보기 모달 표시
     */
    showReportDetail(sessionId) {
        const session = this.loadSession(sessionId);
        if (!session) {
            alert('보고서를 찾을 수 없습니다.');
            return;
        }

        const modal = document.getElementById('reportDetailModal');
        if (!modal) return;

        // 설정 정보 렌더링
        this.renderModalSettings(session.settings);

        // C++ 성능 상세 렌더링
        this.renderModalCppDetails(session.cpp);

        // JS 성능 상세 렌더링
        this.renderModalJsDetails(session.js);

        // 비교 차트 렌더링
        this.renderModalComparisonChart(session.cpp, session.js);

        // 모달 표시
        modal.classList.remove('hidden');

        // 모달 닫기 핸들러 설정
        this.setupModalHandlers();
    }

    /**
     * 모달 설정 정보 렌더링
     */
    renderModalSettings(settings) {
        const container = document.getElementById('modalSettings');
        if (!container) return;

        const filterNames = {
            'none': '없음',
            '0': '컵 속 목소리',
            '1': '무전기',
            '3': '로봇 목소리',
            '4': '메아리',
            '5': '잔향',
            '6': '기타 앰프',
            '7': 'AM 라디오',
            '8': '합창 효과',
            '9': '플랜저',
            '10': '뉴스 인터뷰',
            '11': '범인 목소리'
        };

        container.innerHTML = `
            <div class="modal-setting-item">
                <div class="modal-setting-label">피치</div>
                <div class="modal-setting-value">${settings.pitch || 0}</div>
            </div>
            <div class="modal-setting-item">
                <div class="modal-setting-label">속도</div>
                <div class="modal-setting-value">${settings.timeStretch || 1.0}x</div>
            </div>
            <div class="modal-setting-item">
                <div class="modal-setting-label">필터</div>
                <div class="modal-setting-value">${filterNames[settings.filter] || '없음'}</div>
            </div>
            <div class="modal-setting-item">
                <div class="modal-setting-label">역재생</div>
                <div class="modal-setting-value">${settings.reverse ? '예' : '아니오'}</div>
            </div>
        `;
    }

    /**
     * 모달 C++ 성능 상세 렌더링
     */
    renderModalCppDetails(cppReport) {
        const container = document.getElementById('modalCppDetails');
        if (!container) return;

        if (!cppReport || !cppReport.features) {
            container.innerHTML = '<div class="empty-state"><p>C++ 보고서가 없습니다</p></div>';
            return;
        }

        // 기존 renderCppDetails와 동일한 로직 사용
        const timeline = document.createElement('div');
        timeline.className = 'trace-timeline';

        const totalDuration = cppReport.totalDuration || 0;

        cppReport.features.forEach(feature => {
            this.renderTraceItem(timeline, feature, 0, totalDuration, false);
        });

        container.innerHTML = '';
        container.appendChild(timeline);
    }

    /**
     * 모달 JS 성능 상세 렌더링
     */
    renderModalJsDetails(jsReport) {
        const container = document.getElementById('modalJsDetails');
        if (!container) return;

        if (!jsReport || !jsReport.features) {
            container.innerHTML = '<div class="empty-state"><p>JavaScript 보고서가 없습니다</p></div>';
            return;
        }

        // 기존 renderJsDetails와 동일한 로직 사용
        const timeline = document.createElement('div');
        timeline.className = 'trace-timeline';

        const totalDuration = jsReport.totalDuration || 0;

        jsReport.features.forEach(feature => {
            this.renderTraceItem(timeline, feature, 0, totalDuration, false);
        });

        container.innerHTML = '';
        container.appendChild(timeline);
    }

    /**
     * 모달 비교 차트 렌더링
     */
    renderModalComparisonChart(cppReport, jsReport) {
        const container = document.getElementById('modalComparisonChart');
        if (!container) return;

        if (!cppReport || !jsReport) {
            container.innerHTML = '<div class="empty-state"><p>비교할 보고서가 없습니다</p></div>';
            return;
        }

        // 간단한 비교 차트 렌더링
        const cppTotal = cppReport.totalDuration || 0;
        const jsTotal = jsReport.totalDuration || 0;

        // C++ 기준으로 성능 비율 계산
        let ratioText = 'N/A';
        if (jsTotal > 0 && cppTotal > 0) {
            const ratio = jsTotal / cppTotal;
            const percentDiff = ((ratio - 1) * 100).toFixed(1);
            const timeDiff = Math.abs(jsTotal - cppTotal).toFixed(2);

            if (ratio >= 1.0) {
                // JS가 더 느림 = C++가 더 빠름
                ratioText = `C++가 ${timeDiff}ms 빠름 (${percentDiff}%)`;
            } else {
                // JS가 더 빠름 (거의 없겠지만)
                ratioText = `JS가 ${timeDiff}ms 빠름 (${Math.abs(percentDiff).toFixed(1)}%)`;
            }
        }

        container.innerHTML = `
            <div class="modal-comparison-summary">
                <div class="comparison-item">
                    <div class="comparison-label">C++ 전체 실행 시간</div>
                    <div class="comparison-value cpp">${cppTotal.toFixed(2)}ms</div>
                </div>
                <div class="comparison-item">
                    <div class="comparison-label">JavaScript 전체 실행 시간</div>
                    <div class="comparison-value js">${jsTotal.toFixed(2)}ms</div>
                </div>
                <div class="comparison-item highlight">
                    <div class="comparison-label">성능 차이</div>
                    <div class="comparison-value ratio">${ratioText}</div>
                </div>
            </div>
        `;

        // D3 차트는 생략하고 간단한 비교만 표시
        // 필요하면 나중에 D3 차트를 추가할 수 있습니다
    }

    /**
     * 모달 이벤트 핸들러 설정
     */
    setupModalHandlers() {
        const modal = document.getElementById('reportDetailModal');
        const closeBtn = document.getElementById('closeDetailModal');
        const closeBtnFooter = document.getElementById('closeDetailModalBtn');
        const overlay = modal.querySelector('.modal-overlay');

        const closeModal = () => {
            modal.classList.add('hidden');
        };

        // X 버튼
        if (closeBtn) {
            closeBtn.onclick = closeModal;
        }

        // 닫기 버튼
        if (closeBtnFooter) {
            closeBtnFooter.onclick = closeModal;
        }

        // 오버레이 클릭
        if (overlay) {
            overlay.onclick = closeModal;
        }

        // ESC 키
        const handleEsc = (e) => {
            if (e.key === 'Escape') {
                closeModal();
                document.removeEventListener('keydown', handleEsc);
            }
        };
        document.addEventListener('keydown', handleEsc);
    }
}
