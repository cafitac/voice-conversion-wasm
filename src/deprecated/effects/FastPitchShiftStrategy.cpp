#include "FastPitchShiftStrategy.h"

FastPitchShiftStrategy::FastPitchShiftStrategy()
    : shifter_(new PitchShifter()) {
}

FastPitchShiftStrategy::~FastPitchShiftStrategy() {
}

AudioBuffer FastPitchShiftStrategy::shiftPitch(const AudioBuffer& input, float semitones) {
    return shifter_->shiftPitch(input, semitones);
}
