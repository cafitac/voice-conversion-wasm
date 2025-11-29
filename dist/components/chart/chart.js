/**
 * PitchChart Component
 * Handles D3.js pitch visualization
 */
export class PitchChart {
    constructor(containerId, options = {}) {
        this.containerId = containerId;
        this.container = document.getElementById(containerId);
        this.options = {
            margin: { top: 20, right: 30, bottom: 40, left: 50 },
            animate: true,
            showPoints: false,
            showGrid: true,
            ...options
        };

        this.svg = null;
        this.data = [];
        this.dataType = null; // 'Original' or 'Processed'
        this.scales = {};
        this.tooltip = null;

        if (this.container) {
            this.init();
        }
    }

    init() {
        // Clear any existing content except empty state
        const emptyState = this.container.querySelector('.empty-state');
        if (emptyState) {
            this.emptyState = emptyState;
        }

        // Create tooltip
        this.createTooltip();

        // Listen for window resize
        window.addEventListener('resize', () => this.resize());
    }

    createTooltip() {
        this.tooltip = document.createElement('div');
        this.tooltip.className = 'chart-tooltip';
        this.tooltip.innerHTML = `
            <span class="tooltip-label">Time:</span>
            <span class="tooltip-value time-value"></span>
            <br>
            <span class="tooltip-label">Pitch:</span>
            <span class="tooltip-value pitch-value"></span>
        `;
        this.container.appendChild(this.tooltip);
    }

    getDimensions() {
        const rect = this.container.getBoundingClientRect();
        const { margin } = this.options;
        return {
            width: rect.width - margin.left - margin.right,
            height: rect.height - margin.top - margin.bottom,
            fullWidth: rect.width,
            fullHeight: rect.height
        };
    }

    setData(pitchData, dataType = null) {
        this.data = pitchData.filter(d => d.pitch > 0); // Filter out unvoiced frames
        this.dataType = dataType; // 'Original', 'Processed', or null
        this.render();
    }

    render() {
        if (!this.container || !this.data.length) return;

        // Hide empty state
        if (this.emptyState) {
            this.emptyState.style.display = 'none';
        }

        // Clear existing SVG
        const existingSvg = this.container.querySelector('svg');
        if (existingSvg) {
            existingSvg.remove();
        }

        const dim = this.getDimensions();
        const { margin } = this.options;

        // Create SVG
        this.svg = d3.select(this.container)
            .append('svg')
            .attr('width', dim.fullWidth)
            .attr('height', dim.fullHeight)
            .append('g')
            .attr('transform', `translate(${margin.left},${margin.top})`);

        // Create scales
        this.scales.x = d3.scaleLinear()
            .domain(d3.extent(this.data, d => d.time))
            .range([0, dim.width]);

        this.scales.y = d3.scaleLinear()
            .domain([
                d3.min(this.data, d => d.pitch) * 0.9,
                d3.max(this.data, d => d.pitch) * 1.1
            ])
            .range([dim.height, 0]);

        // Draw grid
        if (this.options.showGrid) {
            this.drawGrid(dim);
        }

        // Draw axes
        this.drawAxes(dim);

        // Draw line
        this.drawLine(dim);

        // Draw points
        if (this.options.showPoints) {
            this.drawPoints();
        }

        // Draw info
        this.drawInfo(dim);
    }

    drawGrid(dim) {
        // Horizontal grid
        this.svg.append('g')
            .attr('class', 'grid')
            .call(d3.axisLeft(this.scales.y)
                .tickSize(-dim.width)
                .tickFormat('')
            );

        // Vertical grid
        this.svg.append('g')
            .attr('class', 'grid')
            .attr('transform', `translate(0,${dim.height})`)
            .call(d3.axisBottom(this.scales.x)
                .tickSize(-dim.height)
                .tickFormat('')
            );
    }

    drawAxes(dim) {
        // X axis
        this.svg.append('g')
            .attr('class', 'axis x-axis')
            .attr('transform', `translate(0,${dim.height})`)
            .call(d3.axisBottom(this.scales.x)
                .ticks(10)
                .tickFormat(d => `${d.toFixed(1)}s`)
            );

        // X axis label
        this.svg.append('text')
            .attr('class', 'axis-label')
            .attr('x', dim.width / 2)
            .attr('y', dim.height + 35)
            .attr('text-anchor', 'middle')
            .text('Time (seconds)');

        // Y axis
        this.svg.append('g')
            .attr('class', 'axis y-axis')
            .call(d3.axisLeft(this.scales.y)
                .ticks(8)
                .tickFormat(d => `${d.toFixed(0)} Hz`)
            );

        // Y axis label
        this.svg.append('text')
            .attr('class', 'axis-label')
            .attr('transform', 'rotate(-90)')
            .attr('x', -dim.height / 2)
            .attr('y', -40)
            .attr('text-anchor', 'middle')
            .text('Pitch (Hz)');
    }

    drawLine(dim) {
        const line = d3.line()
            .x(d => this.scales.x(d.time))
            .y(d => this.scales.y(d.pitch))
            .curve(d3.curveMonotoneX);

        const path = this.svg.append('path')
            .datum(this.data)
            .attr('class', 'pitch-line')
            .attr('d', line);

        if (this.options.animate) {
            const length = path.node().getTotalLength();
            path
                .attr('stroke-dasharray', length)
                .attr('stroke-dashoffset', length)
                .transition()
                .duration(1500)
                .ease(d3.easeQuadOut)
                .attr('stroke-dashoffset', 0);
        }
    }

    drawPoints() {
        const self = this;

        this.svg.selectAll('.data-point')
            .data(this.data)
            .enter()
            .append('circle')
            .attr('class', 'data-point')
            .attr('cx', d => this.scales.x(d.time))
            .attr('cy', d => this.scales.y(d.pitch))
            .attr('r', 3)
            .on('mouseenter', function(event, d) {
                self.showTooltip(event, d);
            })
            .on('mouseleave', () => {
                this.hideTooltip();
            });
    }

    drawInfo(dim) {
        if (!this.data.length) return;

        const avgPitch = d3.mean(this.data, d => d.pitch);
        const minPitch = d3.min(this.data, d => d.pitch);
        const maxPitch = d3.max(this.data, d => d.pitch);
        const duration = d3.max(this.data, d => d.time);

        // Remove existing info
        const existingInfo = this.container.querySelector('.chart-info');
        if (existingInfo) existingInfo.remove();

        const info = document.createElement('div');
        info.className = 'chart-info';

        // Add data type label if available
        let typeLabel = '';
        if (this.dataType) {
            const labelClass = this.dataType === 'Processed' ? 'processed-label' : 'original-label';
            typeLabel = `<div class="chart-info-item ${labelClass}">
                <strong>${this.dataType}</strong>
            </div>`;
        }

        info.innerHTML = `
            ${typeLabel}
            <div class="chart-info-item">
                Duration: <span class="chart-info-value">${duration.toFixed(2)}s</span>
            </div>
            <div class="chart-info-item">
                Avg: <span class="chart-info-value">${avgPitch.toFixed(0)} Hz</span>
            </div>
            <div class="chart-info-item">
                Range: <span class="chart-info-value">${minPitch.toFixed(0)} - ${maxPitch.toFixed(0)} Hz</span>
            </div>
        `;
        this.container.appendChild(info);
    }

    showTooltip(event, d) {
        if (!this.tooltip) return;

        this.tooltip.querySelector('.time-value').textContent = `${d.time.toFixed(3)}s`;
        this.tooltip.querySelector('.pitch-value').textContent = `${d.pitch.toFixed(1)} Hz`;

        const rect = this.container.getBoundingClientRect();
        const x = event.clientX - rect.left + 10;
        const y = event.clientY - rect.top - 10;

        this.tooltip.style.left = `${x}px`;
        this.tooltip.style.top = `${y}px`;
        this.tooltip.classList.add('visible');
    }

    hideTooltip() {
        if (this.tooltip) {
            this.tooltip.classList.remove('visible');
        }
    }

    resize() {
        if (this.data.length) {
            this.render();
        }
    }

    clear() {
        this.data = [];

        // Remove SVG
        const svg = this.container?.querySelector('svg');
        if (svg) svg.remove();

        // Remove info
        const info = this.container?.querySelector('.chart-info');
        if (info) info.remove();

        // Show empty state
        if (this.emptyState) {
            this.emptyState.style.display = '';
        }
    }

    showLoading(text = 'Analyzing...') {
        // Hide empty state
        if (this.emptyState) {
            this.emptyState.style.display = 'none';
        }

        // Remove existing loading
        const existing = this.container?.querySelector('.chart-loading');
        if (existing) existing.remove();

        const loading = document.createElement('div');
        loading.className = 'chart-loading';
        loading.innerHTML = `
            <div class="spinner"></div>
            <div class="chart-loading-text">${text}</div>
        `;
        this.container?.appendChild(loading);
    }

    hideLoading() {
        const loading = this.container?.querySelector('.chart-loading');
        if (loading) loading.remove();
    }
}
