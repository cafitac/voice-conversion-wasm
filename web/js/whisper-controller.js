export class WhisperController {
    constructor() {
        this.whisperModule = null;
        this.modelLoaded = false;
        this.instanceId = null;
    }

    async init() {
        console.log('Loading Whisper module...');

        // Load Whisper WASM module script
        const script = document.createElement('script');
        script.src = 'whisper/libmain.js';
        script.async = true;

        return new Promise((resolve, reject) => {
            script.onload = async () => {
                try {
                    console.log('Whisper script loaded, initializing module...');

                    // Call WhisperModule factory function
                    if (typeof WhisperModule !== 'function') {
                        throw new Error('WhisperModule is not a function');
                    }

                    // Initialize module with configuration
                    this.whisperModule = await WhisperModule({
                        print: (text) => console.log('Whisper:', text),
                        printErr: (text) => console.error('Whisper:', text),
                        onRuntimeInitialized: () => {
                            console.log('Whisper Runtime initialized');
                        }
                    });

                    console.log('Whisper module ready');
                    resolve();
                } catch (error) {
                    console.error('Whisper initialization error:', error);
                    reject(error);
                }
            };
            script.onerror = () => reject(new Error('Failed to load Whisper module'));
            document.head.appendChild(script);
        });
    }

    async loadModel(modelUrl, progressCallback) {
        if (!this.whisperModule) {
            throw new Error('Whisper module not initialized');
        }

        console.log(`Loading Whisper model from ${modelUrl}...`);

        try {
            // Delete old model if exists
            const modelPath = 'whisper.bin';
            try {
                this.whisperModule.FS.unlink(modelPath);
                console.log('Deleted old model file');
            } catch (e) {
                // File doesn't exist, ignore
            }

            // Download model with progress tracking
            const response = await fetch(modelUrl);
            if (!response.ok) {
                throw new Error(`Failed to fetch model: ${response.statusText}`);
            }

            const contentLength = response.headers.get('content-length');
            const totalSize = parseInt(contentLength, 10);
            console.log(`Model size: ${(totalSize / 1024 / 1024).toFixed(1)}MB`);

            // Read response with progress
            const reader = response.body.getReader();
            const chunks = [];
            let receivedLength = 0;

            while (true) {
                const { done, value } = await reader.read();

                if (done) break;

                chunks.push(value);
                receivedLength += value.length;

                // Report progress
                if (progressCallback && totalSize) {
                    const percent = (receivedLength / totalSize) * 100;
                    progressCallback({
                        loaded: receivedLength,
                        total: totalSize,
                        percent: percent
                    });
                }
            }

            // Combine chunks
            const modelData = new Uint8Array(receivedLength);
            let position = 0;
            for (const chunk of chunks) {
                modelData.set(chunk, position);
                position += chunk.length;
            }

            if (progressCallback) {
                progressCallback({ loaded: totalSize, total: totalSize, percent: 100, status: 'Initializing model...' });
            }

            // Store in virtual filesystem
            this.whisperModule.FS.writeFile(modelPath, modelData);

            // Initialize Whisper
            this.instanceId = this.whisperModule.init(modelPath);

            if (this.instanceId === 0) {
                throw new Error('Failed to initialize Whisper');
            }

            this.modelLoaded = true;
            console.log('Whisper model loaded successfully, instance:', this.instanceId);
            return true;
        } catch (error) {
            console.error('Error loading model:', error);
            throw error;
        }
    }

    async transcribe(audioData, sampleRate = 16000, language = 'ko') {
        if (!this.modelLoaded) {
            throw new Error('Model not loaded');
        }

        console.log('Starting transcription...');

        // Convert audio to 16kHz if needed
        const audio16k = await this.resampleTo16kHz(audioData, sampleRate);

        try {
            // Run transcription (now synchronous)
            const result = this.whisperModule.full_default(
                this.instanceId,
                audio16k,
                language,
                1, // threads (single thread for WASM)
                false // translate
            );

            if (result !== 0) {
                throw new Error(`Transcription failed with code: ${result}`);
            }

            console.log('Transcription complete, getting timestamps...');

            // Get word-level timestamps
            const wordTimestamps = this.whisperModule.get_word_timestamps(this.instanceId);

            console.log('Word timestamps retrieved:', wordTimestamps);
            return this.processWordTimestamps(wordTimestamps);
        } catch (error) {
            console.error('Transcription error:', error);
            throw error;
        }
    }

    async resampleTo16kHz(audioData, originalSampleRate) {
        if (originalSampleRate === 16000) {
            return audioData;
        }

        const ratio = originalSampleRate / 16000;
        const newLength = Math.floor(audioData.length / ratio);
        const result = new Float32Array(newLength);

        for (let i = 0; i < newLength; i++) {
            const srcIndex = Math.floor(i * ratio);
            result[i] = audioData[srcIndex];
        }

        return result;
    }

    processWordTimestamps(timestamps) {
        const words = [];
        for (let i = 0; i < timestamps.length; i++) {
            const word = timestamps[i];
            // Convert BigInt to Number before calculations
            const startMs = typeof word.start === 'bigint' ? Number(word.start) : word.start;
            const endMs = typeof word.end === 'bigint' ? Number(word.end) : word.end;

            words.push({
                text: word.text,
                start: startMs / 1000.0, // Convert ms to seconds
                end: endMs / 1000.0,
                duration: (endMs - startMs) / 1000.0,
                probability: word.probability
            });
        }
        return words;
    }

    cleanup() {
        if (this.instanceId && this.whisperModule) {
            this.whisperModule.free(this.instanceId);
            this.instanceId = null;
            this.modelLoaded = false;
        }
    }
}
