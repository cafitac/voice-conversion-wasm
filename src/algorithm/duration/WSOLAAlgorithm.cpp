#include "WSOLAAlgorithm.h"

WSOLAAlgorithm::WSOLAAlgorithm(int windowSize, int hopSize)
    : windowSize_(windowSize), hopSize_(hopSize) {
}

WSOLAAlgorithm::~WSOLAAlgorithm() {
}

AudioBuffer WSOLAAlgorithm::stretch(const AudioBuffer& input, float ratio) {
    // TODO: WSOLA 구현
    // 현재는 원본 반환 (placeholder)
    return input;
}
