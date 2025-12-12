/**
 * WavEncoder - AudioBuffer를 WAV 파일로 인코딩
 */
export class WavEncoder {
    /**
     * AudioBuffer를 WAV Blob으로 변환
     * @param {AudioBuffer} audioBuffer - 변환할 오디오 버퍼
     * @param {number} bitDepth - 비트 뎁스 (기본값: 16)
     * @returns {Blob} WAV 파일 Blob
     */
    static encodeWav(audioBuffer, bitDepth = 16) {
        const sampleRate = audioBuffer.getSampleRate();
        const numChannels = audioBuffer.getChannels();
        const samples = audioBuffer.getData();
        const numSamples = samples.length;

        // WAV 파일 크기 계산
        const bytesPerSample = bitDepth / 8;
        const blockAlign = numChannels * bytesPerSample;
        const byteRate = sampleRate * blockAlign;
        const dataSize = numSamples * bytesPerSample;
        const fileSize = 44 + dataSize; // 44 bytes for header

        // ArrayBuffer 생성
        const buffer = new ArrayBuffer(fileSize);
        const view = new DataView(buffer);

        // WAV 헤더 작성
        let offset = 0;

        // RIFF chunk descriptor
        this.writeString(view, offset, 'RIFF'); offset += 4;
        view.setUint32(offset, fileSize - 8, true); offset += 4; // file size - 8
        this.writeString(view, offset, 'WAVE'); offset += 4;

        // fmt sub-chunk
        this.writeString(view, offset, 'fmt '); offset += 4;
        view.setUint32(offset, 16, true); offset += 4; // fmt chunk size
        view.setUint16(offset, 1, true); offset += 2; // audio format (1 = PCM)
        view.setUint16(offset, numChannels, true); offset += 2; // number of channels
        view.setUint32(offset, sampleRate, true); offset += 4; // sample rate
        view.setUint32(offset, byteRate, true); offset += 4; // byte rate
        view.setUint16(offset, blockAlign, true); offset += 2; // block align
        view.setUint16(offset, bitDepth, true); offset += 2; // bits per sample

        // data sub-chunk
        this.writeString(view, offset, 'data'); offset += 4;
        view.setUint32(offset, dataSize, true); offset += 4;

        // 오디오 데이터 작성
        if (bitDepth === 16) {
            for (let i = 0; i < numSamples; i++) {
                // Float32 [-1.0, 1.0] -> Int16 [-32768, 32767]
                const sample = Math.max(-1, Math.min(1, samples[i]));
                const int16 = sample < 0 ? sample * 32768 : sample * 32767;
                view.setInt16(offset, int16, true);
                offset += 2;
            }
        } else if (bitDepth === 8) {
            for (let i = 0; i < numSamples; i++) {
                // Float32 [-1.0, 1.0] -> Uint8 [0, 255]
                const sample = Math.max(-1, Math.min(1, samples[i]));
                const uint8 = Math.round((sample + 1) * 127.5);
                view.setUint8(offset, uint8);
                offset += 1;
            }
        } else if (bitDepth === 32) {
            for (let i = 0; i < numSamples; i++) {
                // Float32 그대로 저장
                view.setFloat32(offset, samples[i], true);
                offset += 4;
            }
        }

        // Blob 생성
        return new Blob([buffer], { type: 'audio/wav' });
    }

    /**
     * DataView에 문자열 작성
     * @param {DataView} view
     * @param {number} offset
     * @param {string} string
     */
    static writeString(view, offset, string) {
        for (let i = 0; i < string.length; i++) {
            view.setUint8(offset + i, string.charCodeAt(i));
        }
    }

    /**
     * Blob을 다운로드
     * @param {Blob} blob
     * @param {string} filename
     */
    static downloadBlob(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.style.display = 'none';
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();

        // 메모리 정리
        setTimeout(() => {
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        }, 100);
    }
}
