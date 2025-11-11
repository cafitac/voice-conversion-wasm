#include "HighQualityTimeStretchStrategy.h"
#include <cmath>
#include <algorithm>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

HighQualityTimeStretchStrategy::HighQualityTimeStretchStrategy(int frameSize, int hopSize)
    : frameSize_(frameSize), hopSize_(hopSize) {
}

HighQualityTimeStretchStrategy::~HighQualityTimeStretchStrategy() {
}

AudioBuffer HighQualityTimeStretchStrategy::stretch(const AudioBuffer& input, float ratio) {
    if (ratio <= 0.0f) ratio = 1.0f;

    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    int channels = input.getChannels();

    auto stretched = wsolaStretch(inputData, ratio, sampleRate, channels);

    AudioBuffer result(sampleRate, channels);
    result.setData(stretched);

    return result;
}

std::vector<float> HighQualityTimeStretchStrategy::wsolaStretch(
    const std::vector<float>& input,
    float ratio,
    int sampleRate,
    int channels) {

    if (input.empty()) return input;

    // 채널별로 처리 (mono로 가정, stereo는 나중에 확장 가능)
    int samplesPerChannel = input.size() / channels;

    // 출력 크기 계산
    int outputSamplesPerChannel = static_cast<int>(samplesPerChannel * ratio);
    size_t outputSize = outputSamplesPerChannel * channels;
    std::vector<float> output(outputSize, 0.0f);

    // WSOLA 파라미터
    int analysis_hop = hopSize_;
    int synthesis_hop = static_cast<int>(hopSize_ * ratio);
    int overlap_size = frameSize_ - synthesis_hop;  // synthesis_hop 기준으로 계산
    int search_range = hopSize_ / 2;  // 탐색 범위 축소

    int input_pos = 0;
    int output_pos = 0;
    int frame_count = 0;

    while (input_pos + frameSize_ < samplesPerChannel &&
           output_pos + frameSize_ < outputSamplesPerChannel) {

        // 예상 입력 위치 계산 (analysis_hop 기준)
        int expected_input_pos = frame_count * analysis_hop;
        int actual_input_pos = expected_input_pos;

        // 최적 오버랩 위치 찾기 (이전 출력과의 상관관계 최대화)
        if (output_pos > 0 && overlap_size > 0) {
            // 이전 출력의 마지막 부분을 타겟으로
            std::vector<float> target(overlap_size);
            for (int i = 0; i < overlap_size; ++i) {
                int pos = output_pos - overlap_size + i;
                if (pos >= 0 && pos < outputSamplesPerChannel) {
                    target[i] = output[pos * channels];
                } else {
                    target[i] = 0.0f;
                }
            }

            // 예상 위치 근처에서 최적 위치 찾기 (미세 조정)
            int search_start = std::max(0, expected_input_pos - search_range);
            int search_end = std::min(samplesPerChannel - frameSize_, expected_input_pos + search_range);
            int search_length = search_end - search_start;

            if (search_length > 0) {
                int best_offset = findBestMatch(target, input, search_start, search_length);
                if (best_offset >= 0) {
                    actual_input_pos = search_start + best_offset;
                }
            }
        }

        // 현재 프레임 추출 (조정된 위치에서)
        std::vector<float> frame(frameSize_);
        for (int i = 0; i < frameSize_; ++i) {
            if (actual_input_pos + i < samplesPerChannel) {
                frame[i] = input[(actual_input_pos + i) * channels];
            } else {
                frame[i] = 0.0f;
            }
        }

        // Hanning window 적용
        applyWindow(frame);

        // Overlap-add
        overlapAdd(output, frame, output_pos * channels, overlap_size);

        // 다음 위치로 이동
        output_pos += synthesis_hop;
        frame_count++;
    }

    return output;
}

int HighQualityTimeStretchStrategy::findBestMatch(
    const std::vector<float>& target,
    const std::vector<float>& search,
    int searchStart,
    int searchRange) {

    if (target.empty() || search.empty()) return 0;

    int best_offset = 0;
    float max_correlation = -std::numeric_limits<float>::infinity();

    // Cross-correlation 계산
    for (int offset = 0; offset < searchRange; ++offset) {
        int pos = searchStart + offset;
        if (pos + static_cast<int>(target.size()) > static_cast<int>(search.size())) {
            break;
        }

        float correlation = 0.0f;
        for (size_t i = 0; i < target.size(); ++i) {
            correlation += target[i] * search[pos + i];
        }

        if (correlation > max_correlation) {
            max_correlation = correlation;
            best_offset = offset;
        }
    }

    return best_offset;
}

void HighQualityTimeStretchStrategy::applyWindow(std::vector<float>& frame) {
    int size = frame.size();
    for (int i = 0; i < size; ++i) {
        // Hanning window: 0.5 * (1 - cos(2*pi*i/(N-1)))
        float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (size - 1)));
        frame[i] *= window;
    }
}

void HighQualityTimeStretchStrategy::overlapAdd(
    std::vector<float>& output,
    const std::vector<float>& frame,
    int position,
    int overlapSize) {

    int frameSize = frame.size();

    for (int i = 0; i < frameSize && position + i < static_cast<int>(output.size()); ++i) {
        if (i < overlapSize && output[position + i] != 0.0f) {
            // Overlap 영역: crossfade로 부드럽게 연결
            float alpha = static_cast<float>(i) / overlapSize;
            output[position + i] = output[position + i] * (1.0f - alpha) + frame[i] * alpha;
        } else {
            // Non-overlap 영역: 직접 할당
            output[position + i] = frame[i];
        }
    }
}
