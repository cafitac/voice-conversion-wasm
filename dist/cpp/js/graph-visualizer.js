export class GraphVisualizer {
    constructor() {
        this.pitchChart = null;
        this.durationChart = null;
        this.pitchData = [];
        this.durationData = [];
    }

    createPitchChart(canvasId, pitchPoints) {
        const ctx = document.getElementById(canvasId).getContext('2d');

        // 기존 차트 파괴
        if (this.pitchChart) {
            this.pitchChart.destroy();
        }

        this.pitchData = pitchPoints;

        const labels = pitchPoints.map(p => p.time.toFixed(2) + 's');
        const frequencies = pitchPoints.map(p => p.frequency);

        this.pitchChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Pitch (Hz)',
                    data: frequencies,
                    borderColor: 'rgb(102, 126, 234)',
                    backgroundColor: 'rgba(102, 126, 234, 0.1)',
                    tension: 0.4,
                    pointRadius: 4,
                    pointHoverRadius: 6
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    title: {
                        display: true,
                        text: 'Pitch Analysis',
                        font: { size: 16 }
                    },
                    legend: {
                        display: true
                    }
                },
                scales: {
                    y: {
                        beginAtZero: false,
                        title: {
                            display: true,
                            text: 'Frequency (Hz)'
                        }
                    },
                    x: {
                        title: {
                            display: true,
                            text: 'Time (seconds)'
                        }
                    }
                },
                interaction: {
                    mode: 'index',
                    intersect: false
                }
            }
        });
    }

    createDurationChart(canvasId, segments) {
        const ctx = document.getElementById(canvasId).getContext('2d');

        // 기존 차트 파괴
        if (this.durationChart) {
            this.durationChart.destroy();
        }

        this.durationData = segments;

        const labels = segments.map((s, i) => `Segment ${i + 1}`);
        const durations = segments.map(s => s.duration);
        const energies = segments.map(s => s.energy);

        this.durationChart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [
                    {
                        label: 'Duration (s)',
                        data: durations,
                        backgroundColor: 'rgba(72, 187, 120, 0.7)',
                        borderColor: 'rgb(72, 187, 120)',
                        borderWidth: 2,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Energy',
                        data: energies,
                        backgroundColor: 'rgba(237, 137, 54, 0.7)',
                        borderColor: 'rgb(237, 137, 54)',
                        borderWidth: 2,
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    title: {
                        display: true,
                        text: 'Duration & Energy Analysis',
                        font: { size: 16 }
                    },
                    legend: {
                        display: true
                    }
                },
                scales: {
                    y: {
                        type: 'linear',
                        display: true,
                        position: 'left',
                        title: {
                            display: true,
                            text: 'Duration (seconds)'
                        }
                    },
                    y1: {
                        type: 'linear',
                        display: true,
                        position: 'right',
                        title: {
                            display: true,
                            text: 'Energy'
                        },
                        grid: {
                            drawOnChartArea: false
                        }
                    }
                }
            }
        });
    }

    drawWaveform(canvasId, audioData, width = 800, height = 100) {
        const canvas = document.getElementById(canvasId);
        canvas.width = width;
        canvas.height = height;
        const ctx = canvas.getContext('2d');

        // 배경
        ctx.fillStyle = '#1a202c';
        ctx.fillRect(0, 0, width, height);

        // 파형
        ctx.strokeStyle = '#667eea';
        ctx.lineWidth = 2;
        ctx.beginPath();

        const step = Math.ceil(audioData.length / width);
        const amp = height / 2;

        for (let i = 0; i < width; i++) {
            const index = i * step;
            const value = audioData[index] || 0;
            const y = amp + value * amp;

            if (i === 0) {
                ctx.moveTo(i, y);
            } else {
                ctx.lineTo(i, y);
            }
        }

        ctx.stroke();

        // 중심선
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.3)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, amp);
        ctx.lineTo(width, amp);
        ctx.stroke();
    }

    clearCharts() {
        if (this.pitchChart) {
            this.pitchChart.destroy();
            this.pitchChart = null;
        }
        if (this.durationChart) {
            this.durationChart.destroy();
            this.durationChart = null;
        }
    }
}
