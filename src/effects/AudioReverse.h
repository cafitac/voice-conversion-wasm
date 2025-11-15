#ifndef AUDIOREVERSE_H
#define AUDIOREVERSE_H

#include "../audio/AudioBuffer.h"

/**
 * AudioReverse
 *
 * 오디오 역재생 효과
 *
 * 전체 오디오 버퍼를 시간 역순으로 뒤집습니다.
 * 간단하지만 창의적인 효과로 다양한 음향 실험에 활용됩니다.
 */
class AudioReverse {
public:
    AudioReverse();
    ~AudioReverse();

    /**
     * 역재생 적용
     *
     * @param input 입력 오디오 버퍼
     * @return 역재생된 오디오 버퍼
     */
    AudioBuffer applyReverse(const AudioBuffer& input);
};

#endif // AUDIOREVERSE_H
