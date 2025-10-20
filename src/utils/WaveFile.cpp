#include "WaveFile.h"
#include <cstring>
#include <algorithm>

WaveFile::WaveFile() {
}

WaveFile::~WaveFile() {
}

std::vector<uint8_t> WaveFile::saveToMemory(const AudioBuffer& buffer) {
    const auto& samples = buffer.getData();
    int sampleRate = buffer.getSampleRate();
    int channels = buffer.getChannels();
    int numSamples = static_cast<int>(samples.size());

    // WAV 헤더 생성
    auto wavData = createWavHeader(sampleRate, channels, numSamples);

    // PCM 데이터 추가 (16-bit)
    for (float sample : samples) {
        // Float (-1.0 ~ 1.0)를 16-bit PCM으로 변환
        sample = std::max(-1.0f, std::min(1.0f, sample)); // 클리핑 방지
        int16_t pcmSample = static_cast<int16_t>(sample * 32767.0f);
        writeInt16(wavData, pcmSample);
    }

    return wavData;
}

AudioBuffer WaveFile::loadFromMemory(const std::vector<uint8_t>& data) {
    AudioBuffer buffer;

    // 최소 WAV 헤더 크기 확인
    if (data.size() < 44) {
        return buffer;
    }

    // RIFF 헤더 확인
    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F') {
        return buffer;
    }

    // WAVE 포맷 확인
    if (data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E') {
        return buffer;
    }

    // 샘플 레이트와 채널 수 읽기
    int sampleRate = readInt32(data, 24);
    int channels = readInt16(data, 22);
    int bitsPerSample = readInt16(data, 34);

    buffer.setSampleRate(sampleRate);
    buffer.setChannels(channels);

    // PCM 데이터 읽기 (44 바이트 이후부터)
    std::vector<float> samples;
    for (size_t i = 44; i + 1 < data.size(); i += 2) {
        int16_t pcmSample = readInt16(data, i);
        float sample = static_cast<float>(pcmSample) / 32767.0f;
        samples.push_back(sample);
    }

    buffer.setData(samples);
    return buffer;
}

std::vector<uint8_t> WaveFile::createWavHeader(int sampleRate, int channels, int numSamples) {
    std::vector<uint8_t> header;

    int bitsPerSample = 16;
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    int blockAlign = channels * bitsPerSample / 8;
    int dataSize = numSamples * bitsPerSample / 8;
    int chunkSize = 36 + dataSize;

    // RIFF 헤더
    header.push_back('R');
    header.push_back('I');
    header.push_back('F');
    header.push_back('F');
    writeInt32(header, chunkSize);
    header.push_back('W');
    header.push_back('A');
    header.push_back('V');
    header.push_back('E');

    // fmt 청크
    header.push_back('f');
    header.push_back('m');
    header.push_back('t');
    header.push_back(' ');
    writeInt32(header, 16); // fmt 청크 크기
    writeInt16(header, 1);  // PCM 포맷
    writeInt16(header, static_cast<int16_t>(channels));
    writeInt32(header, sampleRate);
    writeInt32(header, byteRate);
    writeInt16(header, static_cast<int16_t>(blockAlign));
    writeInt16(header, static_cast<int16_t>(bitsPerSample));

    // data 청크
    header.push_back('d');
    header.push_back('a');
    header.push_back('t');
    header.push_back('a');
    writeInt32(header, dataSize);

    return header;
}

void WaveFile::writeInt16(std::vector<uint8_t>& data, int16_t value) {
    data.push_back(static_cast<uint8_t>(value & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void WaveFile::writeInt32(std::vector<uint8_t>& data, int32_t value) {
    data.push_back(static_cast<uint8_t>(value & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

int16_t WaveFile::readInt16(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 1 >= data.size()) return 0;
    return static_cast<int16_t>(data[offset] | (data[offset + 1] << 8));
}

int32_t WaveFile::readInt32(const std::vector<uint8_t>& data, size_t offset) {
    if (offset + 3 >= data.size()) return 0;
    return static_cast<int32_t>(
        data[offset] |
        (data[offset + 1] << 8) |
        (data[offset + 2] << 16) |
        (data[offset + 3] << 24)
    );
}
