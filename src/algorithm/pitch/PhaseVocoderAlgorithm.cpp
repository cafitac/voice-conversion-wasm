#include "PhaseVocoderAlgorithm.h"

PhaseVocoderAlgorithm::PhaseVocoderAlgorithm(
    int fftSize,
    int hopSize,
    bool preserveFormant
) : preserveFormant_(preserveFormant) {
    shifter_.reset(new PhaseVocoderPitchShifter(fftSize, hopSize));
    shifter_->setFormantPreservation(preserveFormant);
    shifter_->setAntiAliasing(true);
}

PhaseVocoderAlgorithm::~PhaseVocoderAlgorithm() {
}

AudioBuffer PhaseVocoderAlgorithm::shiftPitch(const AudioBuffer& input, float semitones) {
    return shifter_->shiftPitch(input, semitones);
}

void PhaseVocoderAlgorithm::setFormantPreservation(bool enabled) {
    preserveFormant_ = enabled;
    if (shifter_) {
        shifter_->setFormantPreservation(enabled);
    }
}
