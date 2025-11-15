#include "PSOLAPitchShiftStrategy.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PSOLAPitchShiftStrategy::PSOLAPitchShiftStrategy(int windowSize, int hopSize)
    : m_windowSize(windowSize), m_hopSize(hopSize) {
}

PSOLAPitchShiftStrategy::~PSOLAPitchShiftStrategy() {
}

std::vector<float> PSOLAPitchShiftStrategy::createHanningWindow(int size) {
    std::vector<float> window(size);
    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }
    return window;
}

int PSOLAPitchShiftStrategy::estimatePitchPeriod(
    const std::vector<float>& audio,
    int start,
    int length,
    int minPeriod,
    int maxPeriod
) {
    // 자기상관 계산
    std::vector<float> autocorr(maxPeriod - minPeriod + 1, 0.0f);

    for (int lag = minPeriod; lag <= maxPeriod; ++lag) {
        float sum = 0.0f;
        int count = 0;

        for (int i = 0; i < length - lag; ++i) {
            int idx1 = start + i;
            int idx2 = start + i + lag;

            if (idx1 >= 0 && idx1 < (int)audio.size() &&
                idx2 >= 0 && idx2 < (int)audio.size()) {
                sum += audio[idx1] * audio[idx2];
                count++;
            }
        }

        if (count > 0) {
            autocorr[lag - minPeriod] = sum / count;
        }
    }

    // 최대 자기상관 찾기
    int bestLag = minPeriod;
    float maxCorr = autocorr[0];

    for (int lag = minPeriod; lag <= maxPeriod; ++lag) {
        if (autocorr[lag - minPeriod] > maxCorr) {
            maxCorr = autocorr[lag - minPeriod];
            bestLag = lag;
        }
    }

    return bestLag;
}

std::vector<int> PSOLAPitchShiftStrategy::detectPitchMarks(
    const std::vector<float>& audio,
    int sampleRate
) {
    std::vector<int> marks;

    // 피치 범위: 80Hz ~ 800Hz
    int minPeriod = sampleRate / 800;  // ~60 samples at 48kHz
    int maxPeriod = sampleRate / 80;   // ~600 samples at 48kHz

    int pos = 0;
    int analysisLength = m_windowSize;

    while (pos < (int)audio.size() - maxPeriod) {
        // 현재 위치에서 pitch period 추정
        int period = estimatePitchPeriod(
            audio,
            pos,
            std::min(analysisLength, (int)audio.size() - pos),
            minPeriod,
            maxPeriod
        );

        // 피치 마크 추가
        marks.push_back(pos);

        // 다음 마크로 이동 (추정된 period만큼)
        pos += period;
    }

    return marks;
}

std::vector<float> PSOLAPitchShiftStrategy::psolaShift(
    const std::vector<float>& audio,
    const std::vector<int>& pitchMarks,
    float pitchScale
) {
    if (pitchMarks.size() < 2) {
        return audio;
    }

    std::vector<float> output;
    output.reserve((int)(audio.size() / pitchScale) + 1000);

    // 출력 위치 (pitch scale에 따라 조절)
    float outputPos = 0.0f;

    // 각 pitch mark 사이의 간격을 조절
    for (size_t i = 0; i < pitchMarks.size() - 1; ++i) {
        int currentMark = pitchMarks[i];
        int nextMark = pitchMarks[i + 1];
        int period = nextMark - currentMark;

        // 윈도우 크기 (현재 period의 2배)
        int windowSize = period * 2;
        int windowCenter = currentMark;

        // 윈도우 생성
        auto window = createHanningWindow(windowSize);

        // 윈도우 적용된 신호 추출
        std::vector<float> grain(windowSize, 0.0f);
        for (int j = 0; j < windowSize; ++j) {
            int idx = windowCenter - windowSize / 2 + j;
            if (idx >= 0 && idx < (int)audio.size()) {
                grain[j] = audio[idx] * window[j];
            }
        }

        // 출력 위치에 grain 추가 (overlap-add)
        int outStart = (int)outputPos - windowSize / 2;
        for (int j = 0; j < windowSize; ++j) {
            int outIdx = outStart + j;
            if (outIdx >= 0) {
                // 출력 버퍼 확장
                while (outIdx >= (int)output.size()) {
                    output.push_back(0.0f);
                }
                output[outIdx] += grain[j];
            }
        }

        // 다음 출력 위치 (pitch scale에 따라 간격 조절)
        outputPos += period / pitchScale;
    }

    return output;
}

AudioBuffer PSOLAPitchShiftStrategy::shiftPitch(const AudioBuffer& input, float semitones) {
    if (input.getData().empty()) {
        return AudioBuffer(input.getSampleRate(), input.getChannels());
    }

    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();
    const auto& inputData = input.getData();

    // Pitch scale 계산
    float pitchScale = std::pow(2.0f, semitones / 12.0f);

    std::vector<float> outputData;

    if (channels == 1) {
        // 모노: 직접 처리
        auto pitchMarks = detectPitchMarks(inputData, sampleRate);
        outputData = psolaShift(inputData, pitchMarks, pitchScale);
    } else {
        // 스테레오/멀티채널: 채널별 처리
        int totalSamples = inputData.size() / channels;

        // 채널 분리
        std::vector<std::vector<float>> channelData(channels);
        for (int c = 0; c < channels; ++c) {
            channelData[c].reserve(totalSamples);
            for (int i = 0; i < totalSamples; ++i) {
                channelData[c].push_back(inputData[i * channels + c]);
            }
        }

        // 각 채널 처리
        std::vector<std::vector<float>> processedChannels(channels);
        int maxLength = 0;

        for (int c = 0; c < channels; ++c) {
            auto pitchMarks = detectPitchMarks(channelData[c], sampleRate);
            processedChannels[c] = psolaShift(channelData[c], pitchMarks, pitchScale);
            maxLength = std::max(maxLength, (int)processedChannels[c].size());
        }

        // 채널 합치기 (인터리브)
        outputData.reserve(maxLength * channels);
        for (int i = 0; i < maxLength; ++i) {
            for (int c = 0; c < channels; ++c) {
                if (i < (int)processedChannels[c].size()) {
                    outputData.push_back(processedChannels[c][i]);
                } else {
                    outputData.push_back(0.0f);
                }
            }
        }
    }

    // 정규화 (클리핑 방지)
    float maxVal = 0.0f;
    for (float sample : outputData) {
        maxVal = std::max(maxVal, std::abs(sample));
    }

    if (maxVal > 1.0f) {
        for (float& sample : outputData) {
            sample /= maxVal;
        }
    }

    AudioBuffer result(sampleRate, channels);
    result.setData(outputData);
    return result;
}

const char* PSOLAPitchShiftStrategy::getName() const {
    return "PSOLA Pitch Shift";
}
