#include "PSOLAAlgorithm.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PSOLAAlgorithm::PSOLAAlgorithm(int windowSize, int hopSize)
    : windowSize_(windowSize), hopSize_(hopSize) {
}

PSOLAAlgorithm::~PSOLAAlgorithm() {
}

AudioBuffer PSOLAAlgorithm::shiftPitch(const AudioBuffer& input, float semitones) {
    if (input.getChannels() != 1) {
        // PSOLA는 mono만 지원
        return input;
    }

    const auto& audioData = input.getData();
    int sampleRate = input.getSampleRate();

    // Pitch mark 검출
    std::vector<int> pitchMarks = detectPitchMarks(audioData, sampleRate);

    if (pitchMarks.size() < 2) {
        // Pitch mark 부족 → 원본 반환
        return input;
    }

    // Semitones → pitch scale 변환
    float pitchScale = std::pow(2.0f, semitones / 12.0f);

    // PSOLA shift 적용
    std::vector<float> processedAudio = psolaShift(
        audioData, pitchMarks, pitchScale, sampleRate
    );

    // 결과 AudioBuffer 생성
    AudioBuffer result(sampleRate, 1);
    result.setData(processedAudio);

    return result;
}

std::vector<int> PSOLAAlgorithm::detectPitchMarks(
    const std::vector<float>& audio,
    int sampleRate
) const {
    std::vector<int> marks;

    // Autocorrelation 기반 pitch 검출
    const int minPeriod = sampleRate / 500;  // 최대 500Hz
    const int maxPeriod = sampleRate / 60;   // 최소 60Hz

    int position = 0;
    while (position < static_cast<int>(audio.size()) - maxPeriod) {
        // 현재 위치에서 autocorrelation 계산
        std::vector<float> segment(
            audio.begin() + position,
            audio.begin() + std::min(position + windowSize_, static_cast<int>(audio.size()))
        );

        std::vector<float> autocorr = computeAutocorrelation(segment, maxPeriod);

        // Peak 찾기 (minPeriod ~ maxPeriod 범위)
        int bestLag = minPeriod;
        float maxCorr = autocorr[minPeriod];

        for (int lag = minPeriod + 1; lag < maxPeriod && lag < static_cast<int>(autocorr.size()); ++lag) {
            if (autocorr[lag] > maxCorr) {
                maxCorr = autocorr[lag];
                bestLag = lag;
            }
        }

        // Pitch mark 추가
        marks.push_back(position);
        position += bestLag;
    }

    return marks;
}

std::vector<float> PSOLAAlgorithm::computeAutocorrelation(
    const std::vector<float>& signal,
    int maxLag
) const {
    std::vector<float> autocorr(maxLag, 0.0f);

    for (int lag = 0; lag < maxLag; ++lag) {
        float sum = 0.0f;
        int count = 0;

        for (size_t i = 0; i + lag < signal.size(); ++i) {
            sum += signal[i] * signal[i + lag];
            count++;
        }

        if (count > 0) {
            autocorr[lag] = sum / count;
        }
    }

    return autocorr;
}

std::vector<float> PSOLAAlgorithm::psolaShift(
    const std::vector<float>& audio,
    const std::vector<int>& pitchMarks,
    float pitchScale,
    int sampleRate
) const {
    std::vector<float> output;
    output.reserve(static_cast<size_t>(audio.size() / pitchScale * 1.2));

    float outputPos = 0.0f;

    for (size_t i = 0; i < pitchMarks.size() - 1; ++i) {
        int period = pitchMarks[i + 1] - pitchMarks[i];

        // Grain 추출 (Hanning window 적용)
        int grainSize = period * 2;
        int grainStart = pitchMarks[i] - period / 2;

        std::vector<float> grain(grainSize, 0.0f);
        std::vector<float> window = hanningWindow(grainSize);

        for (int j = 0; j < grainSize; ++j) {
            int idx = grainStart + j;
            if (idx >= 0 && idx < static_cast<int>(audio.size())) {
                grain[j] = audio[idx] * window[j];
            }
        }

        // Output에 overlap-add
        int outputStart = static_cast<int>(outputPos) - period / 2;

        // Output 버퍼 확장
        if (outputStart + grainSize > static_cast<int>(output.size())) {
            output.resize(outputStart + grainSize, 0.0f);
        }

        // Overlap-add
        for (int j = 0; j < grainSize; ++j) {
            int idx = outputStart + j;
            if (idx >= 0 && idx < static_cast<int>(output.size())) {
                output[idx] += grain[j];
            }
        }

        // 다음 출력 위치 (pitch scale 적용)
        outputPos += period / pitchScale;
    }

    return output;
}

std::vector<float> PSOLAAlgorithm::hanningWindow(int size) const {
    std::vector<float> window(size);

    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }

    return window;
}
