export class TimelineRenderer {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.words = [];
        this.maxTime = 0;
    }

    setWords(words, maxTime) {
        this.words = words;
        this.maxTime = maxTime;
    }

    render() {
        const width = this.canvas.width;
        const height = this.canvas.height;

        console.log('Timeline render:', {
            width,
            height,
            words: this.words.length,
            maxTime: this.maxTime
        });

        // Clear canvas
        this.ctx.fillStyle = '#1a1a1a';
        this.ctx.fillRect(0, 0, width, height);

        if (this.words.length === 0) {
            console.warn('No words to render');
            return;
        }

        // Margins (match analysis canvas)
        const marginLeft = 60;
        const marginRight = 20;
        const trackHeight = 40;
        const trackY = (height - trackHeight) / 2;

        const timelineWidth = width - marginLeft - marginRight;

        // Draw each word as a block
        this.ctx.font = '12px Arial';
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';

        this.words.forEach((word, index) => {
            const x = marginLeft + (word.start / this.maxTime) * timelineWidth;
            const blockWidth = ((word.end - word.start) / this.maxTime) * timelineWidth;

            // Color based on index (cycle through colors)
            const hue = (index * 137.5) % 360; // Golden angle for color distribution
            this.ctx.fillStyle = `hsl(${hue}, 60%, 50%)`;
            this.ctx.fillRect(x, trackY, Math.max(blockWidth, 2), trackHeight);

            // Draw text if block is wide enough
            if (blockWidth > 30) {
                this.ctx.fillStyle = '#ffffff';
                this.ctx.fillText(
                    word.text,
                    x + blockWidth / 2,
                    trackY + trackHeight / 2
                );
            }

            // Draw border
            this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.5)';
            this.ctx.lineWidth = 1;
            this.ctx.strokeRect(x, trackY, Math.max(blockWidth, 2), trackHeight);
        });

        // Draw timeline markers (match analysis canvas)
        this.ctx.fillStyle = '#ffffff';
        this.ctx.font = '12px Arial';
        this.ctx.textAlign = 'center';

        for (let i = 0; i <= 4; i++) {
            const time = this.maxTime * i / 4.0;
            const x = marginLeft + (timelineWidth * i / 4.0);

            // Draw tick mark
            this.ctx.fillRect(x - 0.5, 5, 1, 10);

            // Draw time label
            this.ctx.fillText(
                time.toFixed(2) + 's',
                x,
                20
            );
        }

        // Draw label
        this.ctx.font = 'bold 14px Arial';
        this.ctx.textAlign = 'left';
        this.ctx.fillText('Transcription Timeline', marginLeft, height - 10);
    }

    clear() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        this.words = [];
    }
}
