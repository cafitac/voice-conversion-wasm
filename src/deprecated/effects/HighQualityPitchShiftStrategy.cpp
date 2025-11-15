#include "HighQualityPitchShiftStrategy.h"

HighQualityPitchShiftStrategy::HighQualityPitchShiftStrategy(int fftSize, int hopSize)
    : shifter_(new PhaseVocoderPitchShifter(fftSize, hopSize)) {
}

HighQualityPitchShiftStrategy::~HighQualityPitchShiftStrategy() {
}

AudioBuffer HighQualityPitchShiftStrategy::shiftPitch(const AudioBuffer& input, float semitones) {
    return shifter_->shiftPitch(input, semitones);
}

void HighQualityPitchShiftStrategy::setFormantPreservation(bool enabled) {
    shifter_->setFormantPreservation(enabled);
}

void HighQualityPitchShiftStrategy::setAntiAliasing(bool enabled) {
    shifter_->setAntiAliasing(enabled);
}
