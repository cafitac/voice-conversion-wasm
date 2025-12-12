#include "VoiceFilter.h"
#include <cmath>
#include <algorithm>
#include <SoundTouch.h>

VoiceFilter::VoiceFilter() {
}

VoiceFilter::~VoiceFilter() {
}

AudioBuffer VoiceFilter::applyFilter(const AudioBuffer& input, FilterType type, float param1, float param2) {
    // 원본 RMS 계산 (볼륨 보정용)
    float originalRMS = calculateRMS(input.getData());
    
    AudioBuffer result;
    switch (type) {
        case FilterType::LOW_PASS: {
            // 🐻 곰: 아주 낮은 저음 위주 (굵고 둔한 느낌)
            // param1: 0.0 ~ 1.0 -> 약 120Hz ~ 400Hz
            float minCut = 120.0f;
            float maxCut = 400.0f;
            float cutoff = minCut + (maxCut - minCut) * std::clamp(param1, 0.0f, 1.0f);
            result = applyLowPass(input, cutoff);
            break;
        }
        case FilterType::HIGH_PASS: {
            // 🐰 토끼: 아주 높은 고음 위주 (얇고 날카로운 느낌)
            // param1: 0.0 ~ 1.0 -> 약 2500Hz ~ 6000Hz
            float minCut = 2500.0f;
            float maxCut = 6000.0f;
            float cutoff = minCut + (maxCut - minCut) * std::clamp(param1, 0.0f, 1.0f);
            result = applyHighPass(input, cutoff);
            break;
        }
        case FilterType::BAND_PASS: {
            // 📻 라디오: 사람 목소리 대역만 남기는 느낌 (전화기/라디오 톤)
            // lowCutoff:  ~300Hz, highCutoff: ~3kHz 근처를 기본값으로 두고 param으로 미세 조정
            float baseLow = 300.0f;
            float baseHigh = 3000.0f;
            // param1: 저역 쪽 보정 (0.5를 기준으로 ±150Hz)
            float lowOffset = (std::clamp(param1, 0.0f, 1.0f) - 0.5f) * 300.0f;
            // param2: 고역 쪽 보정 (0.5를 기준으로 ±800Hz)
            float highOffset = (std::clamp(param2, 0.0f, 1.0f) - 0.5f) * 1600.0f;
            float lowCutoff = std::max(80.0f, baseLow + lowOffset);
            float highCutoff = std::min(6000.0f, baseHigh + highOffset);
            if (highCutoff <= lowCutoff + 100.0f) {
                highCutoff = lowCutoff + 100.0f;
            }
            result = applyBandPass(input, lowCutoff, highCutoff);
            break;
        }
        case FilterType::ROBOT:
            result = applyRobot(input);
            break;
        case FilterType::ECHO:
            result = applyEcho(input, param1 * 0.5f + 0.1f, param2 * 0.7f + 0.1f);
            break;
        case FilterType::REVERB:
            result = applyReverb(input, param1, param2);
            break;
        case FilterType::DISTORTION:
            result = applyDistortion(input, param1, param2);
            break;
        case FilterType::AM_RADIO:
            result = applyAMRadio(input, param1, param2);
            break;
        case FilterType::CHORUS:
            result = applyChorus(input, param1, param2);
            break;
        case FilterType::FLANGER:
            result = applyFlanger(input, param1, param2);
            break;
        case FilterType::VOICE_CHANGER_MALE_TO_FEMALE:
            result = applyVoiceChangerMaleToFemale(input, param1);
            break;
        case FilterType::VOICE_CHANGER_FEMALE_TO_MALE:
            result = applyVoiceChangerFemaleToMale(input, param1);
            break;
        default:
            return input;
    }
    
    // 필터 적용 후 RMS 계산
    float filteredRMS = calculateRMS(result.getData());
    
    // 볼륨 보정: 원본 RMS에 맞춰 조정 (단, 클리핑 방지)
    if (filteredRMS > 0.0001f && originalRMS > 0.0001f) {
        float gain = originalRMS / filteredRMS;
        // 과도한 증폭 방지 (최대 3배)
        gain = std::min(gain, 3.0f);
        
        auto& data = result.getData();
        size_t i = 0;
        const size_t size = data.size();
        const size_t simdSize = size - (size % 4);

        // Loop Unrolling: 4-way (루프 오버헤드 감소 + 컴파일러 자동 벡터화 유도)
        for (; i < simdSize; i += 4) {
            data[i] = std::max(-1.0f, std::min(1.0f, data[i] * gain));
            data[i+1] = std::max(-1.0f, std::min(1.0f, data[i+1] * gain));
            data[i+2] = std::max(-1.0f, std::min(1.0f, data[i+2] * gain));
            data[i+3] = std::max(-1.0f, std::min(1.0f, data[i+3] * gain));
        }

        // Handle remaining samples
        for (; i < size; ++i) {
            data[i] = std::max(-1.0f, std::min(1.0f, data[i] * gain));
        }
    }
    
    return result;
}

AudioBuffer VoiceFilter::applyLowPass(const AudioBuffer& input, float cutoff) {
    AudioBuffer output = input;
    auto& data = output.getData();
    applySimpleLowPass(data, cutoff, input.getSampleRate());
    return output;
}

AudioBuffer VoiceFilter::applyHighPass(const AudioBuffer& input, float cutoff) {
    AudioBuffer output = input;
    auto& data = output.getData();
    applySimpleHighPass(data, cutoff, input.getSampleRate());
    return output;
}

AudioBuffer VoiceFilter::applyBandPass(const AudioBuffer& input, float lowCutoff, float highCutoff) {
    AudioBuffer output = applyHighPass(input, lowCutoff);
    return applyLowPass(output, highCutoff);
}

AudioBuffer VoiceFilter::applyRobot(const AudioBuffer& input) {
    // 간단한 로봇 효과: 사인파 모듈레이션
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();

    float modFreq = 30.0f; // Hz
    for (size_t i = 0; i < data.size(); ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float modulator = std::sin(2.0f * M_PI * modFreq * t);
        data[i] *= (0.5f + 0.5f * modulator);
    }

    return output;
}

AudioBuffer VoiceFilter::applyEcho(const AudioBuffer& input, float delay, float feedback) {
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();
    int delaySamples = static_cast<int>(delay * sampleRate);

    if (delaySamples >= static_cast<int>(data.size())) {
        return output;
    }

    for (int i = delaySamples; i < static_cast<int>(data.size()); ++i) {
        data[i] += data[i - delaySamples] * feedback;
        // 클리핑 방지
        data[i] = std::max(-1.0f, std::min(1.0f, data[i]));
    }

    return output;
}

AudioBuffer VoiceFilter::applyReverb(const AudioBuffer& input, float roomSize, float damping) {
    // 간단한 리버브: 여러 딜레이의 조합
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();

    // 여러 딜레이 라인
    std::vector<int> delays = {
        static_cast<int>(0.029f * roomSize * sampleRate),
        static_cast<int>(0.037f * roomSize * sampleRate),
        static_cast<int>(0.041f * roomSize * sampleRate),
        static_cast<int>(0.043f * roomSize * sampleRate)
    };

    float feedbackGain = 0.3f * (1.0f - damping);

    for (int delay : delays) {
        if (delay >= static_cast<int>(data.size())) continue;

        for (int i = delay; i < static_cast<int>(data.size()); ++i) {
            data[i] += data[i - delay] * feedbackGain;
            data[i] = std::max(-1.0f, std::min(1.0f, data[i]));
        }
    }

    return output;
}

void VoiceFilter::applySimpleLowPass(std::vector<float>& data, float cutoff, int sampleRate) {
    if (data.size() < 2) return;

    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (rc + dt);

    for (size_t i = 1; i < data.size(); ++i) {
        data[i] = data[i - 1] + alpha * (data[i] - data[i - 1]);
    }
}

void VoiceFilter::applySimpleHighPass(std::vector<float>& data, float cutoff, int sampleRate) {
    if (data.size() < 2) return;

    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = rc / (rc + dt);

    // ✅ Memory optimization: Store only previous values, not full vector copy
    float prevOriginal = data[0];
    float prevOutput = data[0];

    for (size_t i = 1; i < data.size(); ++i) {
        float currentOriginal = data[i];
        data[i] = alpha * (prevOutput + currentOriginal - prevOriginal);
        prevOutput = data[i];
        prevOriginal = currentOriginal;
    }
}

float VoiceFilter::calculateRMS(const std::vector<float>& data) {
    if (data.empty()) return 0.0f;

    float sum = 0.0f;
    size_t i = 0;
    const size_t size = data.size();
    const size_t simdSize = size - (size % 4);

    // Loop Unrolling: 4-way (루프 오버헤드 감소 + 컴파일러 자동 벡터화 유도)
    for (; i < simdSize; i += 4) {
        sum += data[i] * data[i];
        sum += data[i+1] * data[i+1];
        sum += data[i+2] * data[i+2];
        sum += data[i+3] * data[i+3];
    }

    // Handle remaining samples
    for (; i < size; ++i) {
        sum += data[i] * data[i];
    }

    return std::sqrt(sum / size);
}

AudioBuffer VoiceFilter::applyDistortion(const AudioBuffer& input, float drive, float tone) {
    // 🎸 기타 앰프 같은 왜곡 효과
    AudioBuffer output = input;
    auto& data = output.getData();
    
    // Drive: 0.0 ~ 1.0 -> 1.0 ~ 10.0 배 증폭
    float gain = 1.0f + drive * 9.0f;
    
    // Tone: 0.0 ~ 1.0 -> 고역 필터 조정 (0.0 = 어둡게, 1.0 = 밝게)
    float toneCutoff = 2000.0f + tone * 8000.0f;
    
    for (size_t i = 0; i < data.size(); ++i) {
        // 증폭
        float sample = data[i] * gain;
        
        // Soft clipping (tanh 사용)
        sample = std::tanh(sample);
        
        // Tone 조정 (고역 필터)
        if (i > 0) {
            float rc = 1.0f / (2.0f * M_PI * toneCutoff);
            float dt = 1.0f / input.getSampleRate();
            float alpha = dt / (rc + dt);
            sample = data[i - 1] + alpha * (sample - data[i - 1]);
        }
        
        data[i] = sample;
    }
    
    return output;
}

AudioBuffer VoiceFilter::applyAMRadio(const AudioBuffer& input, float noiseLevel, float bandwidth) {
    // 📻 AM 라디오 느낌: 노이즈 + 대역 제한
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();
    
    // 대역 제한: bandwidth 0.0 ~ 1.0 -> 2000Hz ~ 4000Hz
    float lowCut = 200.0f;
    float highCut = 2000.0f + bandwidth * 2000.0f;
    
    // Band pass 필터 적용
    output = applyBandPass(output, lowCut, highCut);
    data = output.getData();
    
    // 노이즈 추가: noiseLevel 0.0 ~ 1.0 -> 0.0 ~ 0.15
    float noiseAmount = noiseLevel * 0.15f;
    
    // 간단한 화이트 노이즈 생성
    static unsigned int seed = 12345;
    for (size_t i = 0; i < data.size(); ++i) {
        // 간단한 랜덤 노이즈
        seed = seed * 1103515245 + 12345;
        float noise = ((seed / 2147483648.0f) - 1.0f) * noiseAmount;
        data[i] += noise;
        data[i] = std::max(-1.0f, std::min(1.0f, data[i]));
    }
    
    return output;
}

AudioBuffer VoiceFilter::applyChorus(const AudioBuffer& input, float rate, float depth) {
    // 🎵 합창 효과: 여러 목소리가 함께 부르는 느낌 (부드럽고 넓은 느낌)
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();
    
    // Rate: 0.0 ~ 1.0 -> 0.1Hz ~ 1.5Hz (느린 변조)
    float modRate = 0.1f + rate * 1.4f;
    
    // Depth: 0.0 ~ 1.0 -> 10ms ~ 30ms 딜레이 (더 긴 딜레이)
    float minDelay = 0.010f; // 10ms
    float maxDelay = minDelay + depth * 0.020f; // 최대 30ms
    int maxDelaySamples = static_cast<int>(maxDelay * sampleRate);
    
    std::vector<float> delayLine(maxDelaySamples + 1, 0.0f);
    int delayIndex = 0;
    
    for (size_t i = 0; i < data.size(); ++i) {
        float t = static_cast<float>(i) / sampleRate;
        
        // LFO (Low Frequency Oscillator)로 딜레이 시간 변조
        float lfo = std::sin(2.0f * M_PI * modRate * t);
        float delayTime = minDelay + (maxDelay - minDelay) * (0.5f + 0.5f * lfo);
        int delaySamples = static_cast<int>(delayTime * sampleRate);
        
        if (delaySamples > 0 && delaySamples <= maxDelaySamples) {
            int readIndex = (delayIndex - delaySamples + maxDelaySamples + 1) % (maxDelaySamples + 1);
            float delayedSample = delayLine[readIndex];
            
            // 딜레이된 신호와 원본을 부드럽게 믹스 (피드백 없음)
            data[i] = data[i] * 0.6f + delayedSample * 0.4f;
            
            // 딜레이 라인 업데이트 (피드백 없이 원본만 저장)
            delayLine[delayIndex] = data[i];
            delayIndex = (delayIndex + 1) % (maxDelaySamples + 1);
        } else {
            delayLine[delayIndex] = data[i];
            delayIndex = (delayIndex + 1) % (maxDelaySamples + 1);
        }
    }
    
    return output;
}

AudioBuffer VoiceFilter::applyFlanger(const AudioBuffer& input, float rate, float depth) {
    // 🌊 플랜저 효과: "우우우우" 날아다니는 느낌 (날카롭고 빠른 느낌)
    AudioBuffer output = input;
    auto& data = output.getData();
    int sampleRate = input.getSampleRate();
    
    // Rate: 0.0 ~ 1.0 -> 0.5Hz ~ 8.0Hz (빠른 변조)
    float modRate = 0.5f + rate * 7.5f;
    
    // Depth: 0.0 ~ 1.0 -> 1ms ~ 12ms 딜레이 (매우 짧은 딜레이)
    float minDelay = 0.001f; // 1ms
    float maxDelay = minDelay + depth * 0.011f; // 최대 12ms
    int maxDelaySamples = static_cast<int>(maxDelay * sampleRate);
    
    std::vector<float> delayLine(maxDelaySamples + 1, 0.0f);
    int delayIndex = 0;
    
    for (size_t i = 0; i < data.size(); ++i) {
        float t = static_cast<float>(i) / sampleRate;
        
        // LFO로 딜레이 시간 변조 (더 빠르고 날카롭게)
        float lfo = std::sin(2.0f * M_PI * modRate * t);
        float delayTime = minDelay + (maxDelay - minDelay) * (0.5f + 0.5f * lfo);
        int delaySamples = static_cast<int>(delayTime * sampleRate);
        
        if (delaySamples > 0 && delaySamples <= maxDelaySamples) {
            int readIndex = (delayIndex - delaySamples + maxDelaySamples + 1) % (maxDelaySamples + 1);
            float delayedSample = delayLine[readIndex];
            
            // 피드백과 믹스 (피드백이 있어서 더 날카로운 느낌)
            float feedbackAmount = 0.4f; // 피드백 강도
            data[i] = data[i] + delayedSample * feedbackAmount;
            data[i] = std::max(-1.0f, std::min(1.0f, data[i]));
            
            // 딜레이 라인 업데이트 (피드백 포함)
            delayLine[delayIndex] = data[i] * 0.6f; // 피드백 감쇠
            delayIndex = (delayIndex + 1) % (maxDelaySamples + 1);
        } else {
            delayLine[delayIndex] = data[i];
            delayIndex = (delayIndex + 1) % (maxDelaySamples + 1);
        }
    }
    
    return output;
}

AudioBuffer VoiceFilter::applyVoiceChangerMaleToFemale(const AudioBuffer& input, float intensity) {
    // 👨→👩 남자 목소리를 여자 목소리로 변환 (얇은 목소리만 나오도록)
    // intensity: 0.0 ~ 1.0 -> 피치 시프트 강도 (0 = 변화 없음, 1 = 최대 변환)
    
    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    
    // 피치 시프트: intensity에 따라 +3 ~ +6 semitones (남->여)
    float pitchShift = 3.0f + intensity * 3.0f; // +3 ~ +6 semitones
    
    // SoundTouch 사용
    soundtouch::SoundTouch st;
    st.setSampleRate(sampleRate);
    st.setChannels(1);
    st.setPitchSemiTones(pitchShift);
    st.setTempo(1.0f);  // 속도 유지
    st.setSetting(SETTING_USE_AA_FILTER, 1);
    st.setSetting(SETTING_AA_FILTER_LENGTH, 64);
    st.setSetting(SETTING_SEQUENCE_MS, 40);
    st.setSetting(SETTING_SEEKWINDOW_MS, 15);
    st.setSetting(SETTING_OVERLAP_MS, 8);
    
    // Process
    std::vector<float> samples(inputData.begin(), inputData.end());
    st.putSamples(samples.data(), samples.size());
    st.flush();
    
    // Retrieve output
    std::vector<float> outputData;
    outputData.resize(samples.size() * 2);  // 여유 공간
    int received = st.receiveSamples(outputData.data(), outputData.size());
    outputData.resize(received);
    
    AudioBuffer result(sampleRate, 1);
    result.setData(outputData);
    
    // 이중으로 들리지 않도록 블렌드 제거, 피치 시프트만 사용
    // 약간의 고역 강조로 더 자연스러운 여성 목소리 느낌 (블렌드 없이)
    if (intensity > 0.5f) {
        // 고역 통과 필터로 약간 밝게 (원본 블렌드 없이)
        float highCut = 1500.0f + intensity * 1500.0f;
        result = applyHighPass(result, highCut);
    }
    
    return result;
}

AudioBuffer VoiceFilter::applyVoiceChangerFemaleToMale(const AudioBuffer& input, float intensity) {
    // 🎭 범인 목소리: 얇은 목소리와 낮은 목소리가 2중으로 들려서 수상해 보이게
    // intensity: 0.0 ~ 1.0 -> 피치 시프트 강도 (0 = 변화 없음, 1 = 최대 변환)
    
    const auto& inputData = input.getData();
    int sampleRate = input.getSampleRate();
    
    // 피치 시프트: intensity에 따라 -4 ~ -7 semitones (더 낮게)
    float pitchShift = -4.0f - intensity * 3.0f; // -4 ~ -7 semitones
    
    // SoundTouch 사용
    soundtouch::SoundTouch st;
    st.setSampleRate(sampleRate);
    st.setChannels(1);
    st.setPitchSemiTones(pitchShift);
    st.setTempo(1.0f);  // 속도 유지
    st.setSetting(SETTING_USE_AA_FILTER, 1);
    st.setSetting(SETTING_AA_FILTER_LENGTH, 64);
    st.setSetting(SETTING_SEQUENCE_MS, 40);
    st.setSetting(SETTING_SEEKWINDOW_MS, 15);
    st.setSetting(SETTING_OVERLAP_MS, 8);
    
    // Process
    std::vector<float> samples(inputData.begin(), inputData.end());
    st.putSamples(samples.data(), samples.size());
    st.flush();
    
    // Retrieve output
    std::vector<float> outputData;
    outputData.resize(samples.size() * 2);  // 여유 공간
    int received = st.receiveSamples(outputData.data(), outputData.size());
    outputData.resize(received);
    
    AudioBuffer result(sampleRate, 1);
    result.setData(outputData);
    
    // 저역 통과 필터로 범인 목소리 느낌
    if (intensity > 0.5f) {
        float lowCut = 600.0f - intensity * 200.0f; // 400Hz ~ 600Hz
        result = applyLowPass(result, lowCut);
    }
    
    // 이중으로 들리게 하기 위해 원본과 블렌드 (수상해 보이게)
    auto& resultData = result.getData();
    for (size_t i = 0; i < std::min(resultData.size(), inputData.size()); ++i) {
        // 낮은 목소리(변환된 것)와 얇은 목소리(원본)를 함께 믹스
        resultData[i] = resultData[i] * 0.6f + inputData[i] * 0.4f;
    }
    
    return result;
}
