#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <vector>
#include <cstdint>

class AudioBuffer {
public:
    AudioBuffer();
    AudioBuffer(int sampleRate, int channels);
    ~AudioBuffer();

    // 오디오 데이터 설정
    void setData(const std::vector<float>& data);
    void appendData(const std::vector<float>& data);
    void clear();

    // 오디오 데이터 가져오기
    const std::vector<float>& getData() const;
    std::vector<float>& getData();

    // 메타데이터
    int getSampleRate() const;
    int getChannels() const;
    size_t getLength() const; // 샘플 수
    float getDuration() const; // 초 단위

    void setSampleRate(int rate);
    void setChannels(int channels);

    // Pitch curve 메타데이터 (Variable pitch shift용)
    void setPitchCurve(const std::vector<float>& curve);
    const std::vector<float>& getPitchCurve() const;
    bool hasPitchCurve() const;
    void clearPitchCurve();

private:
    std::vector<float> data_;
    int sampleRate_;
    int channels_;
    std::vector<float> pitchCurve_;  // 각 샘플의 semitones 값 (variable pitch shift용)
};

#endif // AUDIOBUFFER_H
