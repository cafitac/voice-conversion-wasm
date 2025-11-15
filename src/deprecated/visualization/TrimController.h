#ifndef TRIMCONTROLLER_H
#define TRIMCONTROLLER_H

#include <emscripten/val.h>
#include <string>

class TrimController {
public:
    TrimController();
    ~TrimController();

    void enable(const std::string& canvasId, float maxTime);
    void disable();
    void render();
    void updateTrimPosition(float mouseX, float canvasWidth);
    void startDrag(float mouseX, float canvasWidth);
    void stopDrag();
    void reset();

    float getTrimStart() const { return trimStart_; }
    float getTrimEnd() const { return trimEnd_; }
    bool isEnabled() const { return enabled_; }
    bool isDragging() const { return isDragging_; }

private:
    std::string canvasId_;
    float trimStart_;
    float trimEnd_;
    float maxTime_;
    bool enabled_;
    bool isDragging_;
    int dragHandle_; // 0: none, 1: start, 2: end

    float marginLeft_;
    float marginRight_;

    int getHandleAtPosition(float mouseX, float canvasWidth);
};

#endif // TRIMCONTROLLER_H
