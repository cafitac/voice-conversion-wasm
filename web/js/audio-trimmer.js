export class AudioTrimmer {
    constructor(canvasId, module, uiController) {
        this.canvas = document.getElementById('trimCanvas');
        this.canvasId = 'trimCanvas';
        this.module = module;
        this.uiController = uiController;
        this.enabled = false;
        this.maxTime = 1.0;
        this.trimStart = 0.0;
        this.trimEnd = 1.0;
        this.isDragging = false;
        this.dragHandle = 0; // 0: none, 1: start, 2: end
        this.marginLeft = 60;
        this.marginRight = 20;

        this.setupEventListeners();
    }

    enable(maxTime) {
        this.enabled = true;
        this.maxTime = maxTime;
        this.trimStart = 0.0;
        this.trimEnd = maxTime;
        this.canvas.style.display = 'block';
        this.render();
    }

    disable() {
        this.enabled = false;
        this.canvas.style.display = 'none';
    }

    setupEventListeners() {
        this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
        this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
        this.canvas.addEventListener('mouseup', () => this.onMouseUp());
        this.canvas.addEventListener('mouseleave', () => this.onMouseUp());
    }

    onMouseDown(e) {
        if (!this.enabled) return;

        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;

        this.dragHandle = this.getHandleAtPosition(x);
        if (this.dragHandle > 0) {
            this.isDragging = true;
            console.log('Grabbed handle:', this.dragHandle === 1 ? 'START' : 'END');
        }
    }

    onMouseMove(e) {
        if (!this.enabled) return;

        const rect = this.canvas.getBoundingClientRect();
        const x = e.clientX - rect.left;

        // Update cursor style based on hover
        const hoverHandle = this.getHandleAtPosition(x);
        this.canvas.style.cursor = hoverHandle > 0 ? 'pointer' : 'default';

        // Update position if dragging
        if (this.isDragging) {
            this.updateTrimPosition(x);
            this.render();
            this.updateStatus();
        }
    }

    onMouseUp() {
        this.isDragging = false;
        this.dragHandle = 0;
    }

    getHandleAtPosition(mouseX) {
        const graphWidth = this.canvas.width - this.marginLeft - this.marginRight;
        const startX = this.marginLeft + (this.trimStart / this.maxTime) * graphWidth;
        const endX = this.marginLeft + (this.trimEnd / this.maxTime) * graphWidth;

        // Larger hit area for easier clicking (20px)
        const handleWidth = 20;

        // Check if mouse is within the triangle handle areas
        const distToStart = Math.abs(mouseX - startX);
        const distToEnd = Math.abs(mouseX - endX);

        // Prioritize the closer handle
        if (distToStart <= handleWidth && distToEnd <= handleWidth) {
            // Both handles are close, choose the closer one
            return distToStart <= distToEnd ? 1 : 2;
        } else if (distToStart <= handleWidth) {
            return 1; // start handle
        } else if (distToEnd <= handleWidth) {
            return 2; // end handle
        }

        return 0; // no handle
    }

    updateTrimPosition(mouseX) {
        const graphWidth = this.canvas.width - this.marginLeft - this.marginRight;
        const time = ((mouseX - this.marginLeft) / graphWidth) * this.maxTime;
        const clampedTime = Math.max(0, Math.min(this.maxTime, time));

        console.log('updateTrimPosition - dragHandle:', this.dragHandle, 'mouseX:', mouseX, 'time:', time, 'clampedTime:', clampedTime);

        if (this.dragHandle === 1) {
            // Start handle - cannot go past end
            const newStart = Math.min(clampedTime, this.trimEnd - 0.1);
            console.log('START: old=', this.trimStart, 'new=', newStart);
            this.trimStart = newStart;
        } else if (this.dragHandle === 2) {
            // End handle - cannot go before start
            const newEnd = Math.max(clampedTime, this.trimStart + 0.1);
            console.log('END: old=', this.trimEnd, 'new=', newEnd, 'clampedTime=', clampedTime, 'trimStart+0.1=', this.trimStart + 0.1);
            this.trimEnd = newEnd;
        }
    }

    render() {
        if (!this.enabled) return;

        const ctx = this.canvas.getContext('2d');
        const width = this.canvas.width;
        const height = this.canvas.height;

        // Clear canvas
        ctx.clearRect(0, 0, width, height);

        // Draw background
        ctx.fillStyle = '#1a1a1a';
        ctx.fillRect(0, 0, width, height);

        const graphWidth = width - this.marginLeft - this.marginRight;
        const startX = this.marginLeft + (this.trimStart / this.maxTime) * graphWidth;
        const endX = this.marginLeft + (this.trimEnd / this.maxTime) * graphWidth;

        // Draw timeline bar
        ctx.fillStyle = '#333';
        ctx.fillRect(this.marginLeft, height / 2 - 3, graphWidth, 6);

        // Draw selected region
        ctx.fillStyle = '#4CAF50';
        ctx.fillRect(startX, height / 2 - 4, endX - startX, 8);

        // Draw start handle (larger triangle pointing down)
        ctx.fillStyle = '#FFD700';
        ctx.beginPath();
        ctx.moveTo(startX, 5);
        ctx.lineTo(startX - 12, 28);
        ctx.lineTo(startX + 12, 28);
        ctx.closePath();
        ctx.fill();

        // Add border to start handle for better visibility
        ctx.strokeStyle = '#FFA500';
        ctx.lineWidth = 2;
        ctx.stroke();

        // Draw end handle (larger triangle pointing down)
        ctx.fillStyle = '#FFD700';
        ctx.beginPath();
        ctx.moveTo(endX, 5);
        ctx.lineTo(endX - 12, 28);
        ctx.lineTo(endX + 12, 28);
        ctx.closePath();
        ctx.fill();

        // Add border to end handle for better visibility
        ctx.strokeStyle = '#FFA500';
        ctx.lineWidth = 2;
        ctx.stroke();

        // Draw time labels
        ctx.fillStyle = '#fff';
        ctx.font = '12px Arial';
        ctx.textAlign = 'center';
        ctx.fillText(`${this.trimStart.toFixed(2)}s`, startX, height - 10);
        ctx.fillText(`${this.trimEnd.toFixed(2)}s`, endX, height - 10);
    }

    reset() {
        this.trimStart = 0.0;
        this.trimEnd = this.maxTime;
        this.isDragging = false;
        this.dragHandle = 0;
        this.render();
    }

    updateStatus() {
        const duration = this.trimEnd - this.trimStart;
        const statusEl = document.getElementById('trimStatus');
        if (statusEl) {
            statusEl.textContent = `선택 영역: ${this.trimStart.toFixed(2)}s ~ ${this.trimEnd.toFixed(2)}s (${duration.toFixed(2)}s)`;
        }
    }

    getTrimRange() {
        return {
            start: this.trimStart,
            end: this.trimEnd
        };
    }
}
