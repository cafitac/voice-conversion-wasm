#ifndef WAVEFILE_H
#define WAVEFILE_H

#include "../audio/AudioBuffer.h"
#include <string>
#include <vector>
#include <cstdint>

class WaveFile {
public:
    WaveFile();
    ~WaveFile();

    // WAV 파일로 저장 (Emscripten에서 메모리에 저장 후 JavaScript로 전달)
    std::vector<uint8_t> saveToMemory(const AudioBuffer& buffer);

    // WAV 파일에서 로드 (메모리에서)
    AudioBuffer loadFromMemory(const std::vector<uint8_t>& data);

    // WAV 헤더 생성
    static std::vector<uint8_t> createWavHeader(int sampleRate, int channels, int numSamples);

private:
    // Little-endian 변환
    static void writeInt16(std::vector<uint8_t>& data, int16_t value);
    static void writeInt32(std::vector<uint8_t>& data, int32_t value);

    static int16_t readInt16(const std::vector<uint8_t>& data, size_t offset);
    static int32_t readInt32(const std::vector<uint8_t>& data, size_t offset);
};

#endif // WAVEFILE_H
