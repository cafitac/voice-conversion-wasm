#include "TrimController.h"
#include "CanvasRenderer.h"
#include <cmath>
#include <algorithm>
#include <emscripten/emscripten.h>

using namespace emscripten;

TrimController::TrimController()
    : trimStart_(0.0f), trimEnd_(1.0f), maxTime_(1.0f),
      enabled_(false), isDragging_(false), dragHandle_(0),
      marginLeft_(60.0f), marginRight_(20.0f) {
}

TrimController::~TrimController() {
}

void TrimController::enable(const std::string& canvasId, float maxTime) {
    canvasId_ = canvasId;
    maxTime_ = maxTime;
    trimStart_ = 0.0f;
    trimEnd_ = maxTime;
    enabled_ = true;
}

void TrimController::disable() {
    enabled_ = false;
    isDragging_ = false;
    dragHandle_ = 0;
}

void TrimController::render() {
    if (!enabled_) return;

    CanvasRenderer renderer;
    renderer.drawTrimHandles(canvasId_, trimStart_, trimEnd_, maxTime_);
}

int TrimController::getHandleAtPosition(float mouseX, float canvasWidth) {
    float graphWidth = canvasWidth - marginLeft_ - marginRight_;
    float startX = marginLeft_ + (trimStart_ / maxTime_) * graphWidth;
    float endX = marginLeft_ + (trimEnd_ / maxTime_) * graphWidth;

    float handleSize = 20.0f;

    float distToEnd = std::abs(mouseX - endX);
    float distToStart = std::abs(mouseX - startX);

    if (distToEnd < handleSize && distToEnd <= distToStart) {
        return 2; // end handle
    } else if (distToStart < handleSize) {
        return 1; // start handle
    }

    return 0; // no handle
}

void TrimController::startDrag(float mouseX, float canvasWidth) {
    if (!enabled_) return;

    dragHandle_ = getHandleAtPosition(mouseX, canvasWidth);
    if (dragHandle_ > 0) {
        isDragging_ = true;
    }
}

void TrimController::stopDrag() {
    isDragging_ = false;
    dragHandle_ = 0;
}

void TrimController::reset() {
    trimStart_ = 0.0f;
    trimEnd_ = maxTime_;
    isDragging_ = false;
    dragHandle_ = 0;
}

void TrimController::updateTrimPosition(float mouseX, float canvasWidth) {
    if (!enabled_ || !isDragging_ || dragHandle_ == 0) return;

    float graphWidth = canvasWidth - marginLeft_ - marginRight_;
    float time = ((mouseX - marginLeft_) / graphWidth) * maxTime_;
    float clampedTime = std::max(0.0f, std::min(maxTime_, time));

    if (dragHandle_ == 1) {
        // Start handle - cannot go past end
        trimStart_ = std::min(clampedTime, trimEnd_ - 0.1f);
    } else if (dragHandle_ == 2) {
        // End handle - cannot go before start
        trimEnd_ = std::max(clampedTime, trimStart_ + 0.1f);
    }

    // Debug log
    EM_ASM({
        console.log('Trim update: handle=' + $0 + ', start=' + $1 + ', end=' + $2);
    }, dragHandle_, trimStart_, trimEnd_);
}
