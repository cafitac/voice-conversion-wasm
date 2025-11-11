/**
 * PitchAnalyzer 단위 테스트
 *
 * 사용법:
 *   ./test_pitch_analyzer original.wav
 *
 * 결과:
 *   - Pitch 분석 결과를 콘솔에 출력
 *   - CSV 파일로 저장 (pitch_analysis.csv)
 */

#include "src/audio/AudioBuffer.h"
#include "src/analysis/PitchAnalyzer.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>

// 간단한 WAV 파일 로더 (네이티브 파일시스템용)
class SimpleWavLoader {
public:
    static AudioBuffer loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "파일을 열 수 없습니다: " << filename << std::endl;
            return AudioBuffer();
        }

        // WAV 헤더 읽기
        char chunkId[4];
        uint32_t chunkSize;
        char format[4];

        file.read(chunkId, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        file.read(format, 4);

        if (std::strncmp(chunkId, "RIFF", 4) != 0 || std::strncmp(format, "WAVE", 4) != 0) {
            std::cerr << "올바른 WAV 파일이 아닙니다." << std::endl;
            return AudioBuffer();
        }

        // fmt 청크 찾기
        char subchunk1Id[4];
        uint32_t subchunk1Size;
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;

        file.read(subchunk1Id, 4);
        file.read(reinterpret_cast<char*>(&subchunk1Size), 4);
        file.read(reinterpret_cast<char*>(&audioFormat), 2);
        file.read(reinterpret_cast<char*>(&numChannels), 2);
        file.read(reinterpret_cast<char*>(&sampleRate), 4);
        file.read(reinterpret_cast<char*>(&byteRate), 4);
        file.read(reinterpret_cast<char*>(&blockAlign), 2);
        file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

        // 추가 fmt 데이터 건너뛰기
        if (subchunk1Size > 16) {
            file.seekg(subchunk1Size - 16, std::ios::cur);
        }

        // data 청크 찾기
        char subchunk2Id[4];
        uint32_t subchunk2Size;
        file.read(subchunk2Id, 4);
        file.read(reinterpret_cast<char*>(&subchunk2Size), 4);

        std::cout << "=== WAV 파일 정보 ===" << std::endl;
        std::cout << "샘플레이트: " << sampleRate << " Hz" << std::endl;
        std::cout << "채널 수: " << numChannels << std::endl;
        std::cout << "비트 깊이: " << bitsPerSample << " bits" << std::endl;
        std::cout << "데이터 크기: " << subchunk2Size << " bytes" << std::endl;

        // 오디오 데이터 읽기
        size_t numSamples = subchunk2Size / (bitsPerSample / 8);
        std::vector<float> samples(numSamples);

        if (bitsPerSample == 16) {
            for (size_t i = 0; i < numSamples; ++i) {
                int16_t sample;
                file.read(reinterpret_cast<char*>(&sample), 2);
                samples[i] = static_cast<float>(sample) / 32768.0f;
            }
        } else if (bitsPerSample == 8) {
            for (size_t i = 0; i < numSamples; ++i) {
                uint8_t sample;
                file.read(reinterpret_cast<char*>(&sample), 1);
                samples[i] = (static_cast<float>(sample) - 128.0f) / 128.0f;
            }
        }

        file.close();

        std::cout << "총 샘플 수: " << samples.size() << std::endl;
        std::cout << "재생 시간: " << static_cast<float>(samples.size()) / (sampleRate * numChannels) << " 초" << std::endl;
        std::cout << std::endl;

        AudioBuffer buffer(sampleRate, numChannels);
        buffer.setData(samples);
        return buffer;
    }
};

// Pitch 분석 결과를 CSV로 저장
void savePitchToCsv(const std::vector<PitchPoint>& pitches, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "CSV 파일을 생성할 수 없습니다: " << filename << std::endl;
        return;
    }

    file << "Time(s),Frequency(Hz),Confidence\n";
    for (const auto& point : pitches) {
        file << point.time << "," << point.frequency << "," << point.confidence << "\n";
    }

    file.close();
    std::cout << "결과를 저장했습니다: " << filename << std::endl;
}

// 통계 정보 출력
void printStatistics(const std::vector<PitchPoint>& pitches) {
    if (pitches.empty()) {
        std::cout << "분석된 Pitch 데이터가 없습니다." << std::endl;
        return;
    }

    float minFreq = pitches[0].frequency;
    float maxFreq = pitches[0].frequency;
    float sumFreq = 0.0f;
    float sumConfidence = 0.0f;
    int validCount = 0;

    for (const auto& point : pitches) {
        if (point.frequency > 0.0f) {
            minFreq = std::min(minFreq, point.frequency);
            maxFreq = std::max(maxFreq, point.frequency);
            sumFreq += point.frequency;
            validCount++;
        }
        sumConfidence += point.confidence;
    }

    std::cout << "=== Pitch 분석 통계 ===" << std::endl;
    std::cout << "총 프레임 수: " << pitches.size() << std::endl;
    std::cout << "유효 Pitch 수: " << validCount << std::endl;
    std::cout << "최소 주파수: " << minFreq << " Hz" << std::endl;
    std::cout << "최대 주파수: " << maxFreq << " Hz" << std::endl;
    std::cout << "평균 주파수: " << (validCount > 0 ? sumFreq / validCount : 0.0f) << " Hz" << std::endl;
    std::cout << "평균 신뢰도: " << sumConfidence / pitches.size() << std::endl;
    std::cout << std::endl;
}

// 샘플 데이터 출력 (처음 10개)
void printSampleData(const std::vector<PitchPoint>& pitches, int count = 10) {
    std::cout << "=== 샘플 데이터 (처음 " << count << "개) ===" << std::endl;
    std::cout << "Time(s)\tFreq(Hz)\tConfidence" << std::endl;

    int limit = std::min(count, static_cast<int>(pitches.size()));
    for (int i = 0; i < limit; ++i) {
        const auto& point = pitches[i];
        std::cout << point.time << "\t"
                  << point.frequency << "\t"
                  << point.confidence << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "    PitchAnalyzer 단위 테스트" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 파일 경로 확인
    std::string filename = "original.wav";
    if (argc > 1) {
        filename = argv[1];
    }

    std::cout << "테스트 파일: " << filename << std::endl;
    std::cout << std::endl;

    // 1. WAV 파일 로드
    std::cout << "[1/3] WAV 파일 로딩 중..." << std::endl;
    AudioBuffer buffer = SimpleWavLoader::loadFromFile(filename);

    if (buffer.getData().empty()) {
        std::cerr << "오디오 데이터를 로드할 수 없습니다." << std::endl;
        return 1;
    }

    // 2. Pitch 분석
    std::cout << "[2/3] Pitch 분석 중..." << std::endl;
    PitchAnalyzer analyzer;

    // 분석 파라미터 설정
    analyzer.setMinFrequency(80.0f);   // 80 Hz (남성 저음)
    analyzer.setMaxFrequency(400.0f);  // 400 Hz (여성 고음)

    auto pitches = analyzer.analyze(buffer, 0.02f);  // 20ms 프레임
    std::cout << "분석 완료! " << pitches.size() << "개의 Pitch 포인트를 추출했습니다." << std::endl;
    std::cout << std::endl;

    // 3. 결과 출력 및 저장
    std::cout << "[3/3] 결과 처리 중..." << std::endl;
    printStatistics(pitches);
    printSampleData(pitches, 10);
    savePitchToCsv(pitches, "pitch_analysis.csv");

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "테스트 완료!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
