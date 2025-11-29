#ifndef AUDIOREVERSER_H
#define AUDIOREVERSER_H

#include "../audio/AudioBuffer.h"

class AudioReverser {
public:
    AudioReverser();
    ~AudioReverser();

    // 오디오 역재생
    AudioBuffer reverse(const AudioBuffer& input);
};

#endif // AUDIOREVERSER_H
