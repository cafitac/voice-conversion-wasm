#ifndef CANVASRENDERER_H
#define CANVASRENDERER_H

#include <emscripten/val.h>
#include <string>
#include <vector>
#include "../analysis/PitchAnalyzer.h"
#include "../analysis/DurationAnalyzer.h"

class CanvasRenderer {
public:
    CanvasRenderer();
    ~CanvasRenderer();

    // Duration 막대 그래프 + Pitch 곡선을 통합하여 그리기
    void drawCombinedAnalysis(
        const std::string& canvasId,
        const std::vector<DurationSegment>& segments,
        const std::vector<PitchPoint>& pitchPoints,
        int sampleRate
    );

private:
    // Canvas 2D context 헬퍼 함수들
    void setFillStyle(emscripten::val ctx, const std::string& color);
    void setStrokeStyle(emscripten::val ctx, const std::string& color);
    void setLineWidth(emscripten::val ctx, float width);
    void fillRect(emscripten::val ctx, float x, float y, float width, float height);
    void strokeRect(emscripten::val ctx, float x, float y, float width, float height);
    void drawLine(emscripten::val ctx, float x1, float y1, float x2, float y2);
    void fillText(emscripten::val ctx, const std::string& text, float x, float y);
    void setFont(emscripten::val ctx, const std::string& font);

    // 유틸리티
    float mapValue(float value, float inMin, float inMax, float outMin, float outMax);
};

#endif // CANVASRENDERER_H
