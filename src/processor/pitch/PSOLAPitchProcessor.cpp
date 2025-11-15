#include "PSOLAPitchProcessor.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PSOLAPitchProcessor::PSOLAPitchProcessor(int windowSize, int hopSize)
    : m_windowSize(windowSize), m_hopSize(hopSize) {
}

PSOLAPitchProcessor::~PSOLAPitchProcessor() {
}

std::vector<FrameData> PSOLAPitchProcessor::process(
    const std::vector<FrameData>& frames,
    int sampleRate
) {
    if (frames.empty()) {
        return frames;
    }

    // 1. FrameData → 연속 오디오 변환
    std::vector<float> audio = framesToAudio(frames);

    if (audio.empty()) {
        return frames;
    }

    // 2. Pitch mark 검출
    std::vector<int> pitchMarks = detectPitchMarks(audio, sampleRate);

    if (pitchMarks.size() < 2) {
        return frames;  // Pitch mark가 충분하지 않으면 원본 반환
    }

    // 3. 각 pitch mark의 pitch scale 계산
    std::vector<float> pitchScales;
    pitchScales.reserve(pitchMarks.size());

    for (size_t i = 0; i < pitchMarks.size(); ++i) {
        // 이 pitch mark의 시간
        float time = static_cast<float>(pitchMarks[i]) / sampleRate;

        // FrameData에서 해당 시간의 pitchSemitones 가져오기
        float semitones = getPitchSemitonesAtTime(time, frames);

        // Semitones → pitch scale 변환
        float scale = std::pow(2.0f, semitones / 12.0f);
        pitchScales.push_back(scale);
    }

    // 4. Variable PSOLA shift 적용
    std::vector<float> processedAudio = psolaShiftVariable(
        audio, pitchMarks, pitchScales, sampleRate
    );

    // 5. 처리된 오디오 → FrameData 변환
    std::vector<FrameData> result = audioToFrames(processedAudio, frames, sampleRate);

    return result;
}

std::vector<float> PSOLAPitchProcessor::framesToAudio(
    const std::vector<FrameData>& frames
) const {
    std::vector<float> audio;

    for (const auto& frame : frames) {
        audio.insert(audio.end(), frame.samples.begin(), frame.samples.end());
    }

    return audio;
}

std::vector<FrameData> PSOLAPitchProcessor::audioToFrames(
    const std::vector<float>& audio,
    const std::vector<FrameData>& originalFrames,
    int sampleRate
) const {
    if (originalFrames.empty()) {
        return {};
    }

    std::vector<FrameData> result;

    // 원본 프레임 구조 유지
    size_t audioOffset = 0;
    for (const auto& originalFrame : originalFrames) {
        FrameData newFrame = originalFrame;  // 메타데이터 복사

        // 샘플 복사
        size_t frameSize = originalFrame.samples.size();
        newFrame.samples.clear();

        for (size_t i = 0; i < frameSize && audioOffset < audio.size(); ++i) {
            newFrame.samples.push_back(audio[audioOffset++]);
        }

        // 프레임이 부족하면 0으로 채움
        while (newFrame.samples.size() < frameSize) {
            newFrame.samples.push_back(0.0f);
        }

        result.push_back(newFrame);
    }

    return result;
}

float PSOLAPitchProcessor::getPitchSemitonesAtTime(
    float time,
    const std::vector<FrameData>& frames
) const {
    if (frames.empty()) {
        return 0.0f;
    }

    // 정확히 일치하는 프레임 찾기
    for (const auto& frame : frames) {
        if (std::abs(frame.time - time) < 0.001f) {
            return frame.pitchSemitones;
        }
    }

    // 선형 보간
    const FrameData* before = nullptr;
    const FrameData* after = nullptr;

    for (size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].time <= time) {
            before = &frames[i];
        }
        if (frames[i].time >= time && after == nullptr) {
            after = &frames[i];
            break;
        }
    }

    if (before && after && before != after) {
        float t = (time - before->time) / (after->time - before->time);
        return before->pitchSemitones + t * (after->pitchSemitones - before->pitchSemitones);
    }

    if (before) return before->pitchSemitones;
    if (after) return after->pitchSemitones;

    return 0.0f;
}

std::vector<float> PSOLAPitchProcessor::createHanningWindow(int size) const {
    std::vector<float> window(size);
    for (int i = 0; i < size; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
    }
    return window;
}

int PSOLAPitchProcessor::estimatePitchPeriod(
    const std::vector<float>& audio,
    int start,
    int length,
    int minPeriod,
    int maxPeriod
) const {
    // Autocorrelation 계산
    std::vector<float> autocorr(maxPeriod - minPeriod + 1, 0.0f);

    for (int lag = minPeriod; lag <= maxPeriod; ++lag) {
        float sum = 0.0f;
        int count = 0;

        for (int i = 0; i < length - lag; ++i) {
            int idx1 = start + i;
            int idx2 = start + i + lag;

            if (idx1 >= 0 && idx1 < static_cast<int>(audio.size()) &&
                idx2 >= 0 && idx2 < static_cast<int>(audio.size())) {
                sum += audio[idx1] * audio[idx2];
                count++;
            }
        }

        if (count > 0) {
            autocorr[lag - minPeriod] = sum / count;
        }
    }

    // 최대 autocorrelation 찾기
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

std::vector<int> PSOLAPitchProcessor::detectPitchMarks(
    const std::vector<float>& audio,
    int sampleRate
) const {
    std::vector<int> marks;

    // 피치 범위: 80Hz ~ 800Hz
    int minPeriod = sampleRate / 800;  // ~60 samples at 48kHz
    int maxPeriod = sampleRate / 80;   // ~600 samples at 48kHz

    int pos = 0;
    int analysisLength = m_windowSize;

    while (pos < static_cast<int>(audio.size()) - maxPeriod) {
        // 현재 위치에서 pitch period 추정
        int period = estimatePitchPeriod(
            audio,
            pos,
            std::min(analysisLength, static_cast<int>(audio.size()) - pos),
            minPeriod,
            maxPeriod
        );

        // Pitch mark 추가
        marks.push_back(pos);

        // 다음 mark로 이동
        pos += period;
    }

    return marks;
}

std::vector<float> PSOLAPitchProcessor::psolaShiftVariable(
    const std::vector<float>& audio,
    const std::vector<int>& pitchMarks,
    const std::vector<float>& pitchScales,
    int sampleRate
) const {
    if (pitchMarks.size() < 2 || pitchScales.size() != pitchMarks.size()) {
        return audio;
    }

    std::vector<float> output;
    output.reserve(static_cast<int>(audio.size() * 1.5f));

    // 출력 위치 (pitch scale에 따라 조절)
    float outputPos = 0.0f;

    // 각 pitch mark 사이의 간격을 조절
    for (size_t i = 0; i < pitchMarks.size() - 1; ++i) {
        int currentMark = pitchMarks[i];
        int nextMark = pitchMarks[i + 1];
        int period = nextMark - currentMark;

        // 이 구간의 pitch scale (현재와 다음의 평균)
        float scale = (pitchScales[i] + pitchScales[i + 1]) / 2.0f;

        // 윈도우 크기
        int windowSize = period * 2;
        int windowCenter = currentMark;

        // 윈도우 생성
        auto window = createHanningWindow(windowSize);

        // 윈도우 적용된 신호 추출 (grain)
        std::vector<float> grain(windowSize, 0.0f);
        for (int j = 0; j < windowSize; ++j) {
            int idx = windowCenter - windowSize / 2 + j;
            if (idx >= 0 && idx < static_cast<int>(audio.size())) {
                grain[j] = audio[idx] * window[j];
            }
        }

        // 출력 위치에 grain 추가 (overlap-add)
        int outStart = static_cast<int>(outputPos) - windowSize / 2;
        for (int j = 0; j < windowSize; ++j) {
            int outIdx = outStart + j;
            if (outIdx >= 0) {
                // 출력 버퍼 확장
                while (outIdx >= static_cast<int>(output.size())) {
                    output.push_back(0.0f);
                }
                output[outIdx] += grain[j];
            }
        }

        // 다음 출력 위치 (variable pitch scale에 따라 간격 조절)
        outputPos += period / scale;
    }

    return output;
}
