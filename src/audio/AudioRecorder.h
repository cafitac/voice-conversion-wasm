#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include "AudioBuffer.h"
#include <memory>

class AudioRecorder {
public:
    AudioRecorder(int sampleRate = 44100, int channels = 1);
    ~AudioRecorder();

    // 녹음 데이터 추가 (JavaScript에서 호출)
    void addAudioData(uintptr_t dataPtr, int length);

    // 녹음 시작/종료
    void startRecording();
    void stopRecording();
    bool isRecording() const;

    // 녹음된 데이터 가져오기
    const AudioBuffer& getRecordedAudio() const;
    void clearRecording();

private:
    std::unique_ptr<AudioBuffer> buffer_;
    bool isRecording_;
};

#endif // AUDIORECORDER_H
