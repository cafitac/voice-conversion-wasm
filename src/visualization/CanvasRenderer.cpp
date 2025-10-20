#include "CanvasRenderer.h"
#include <emscripten/emscripten.h>
#include <algorithm>
#include <cmath>

using namespace emscripten;

CanvasRenderer::CanvasRenderer() {
}

CanvasRenderer::~CanvasRenderer() {
}

void CanvasRenderer::drawCombinedAnalysis(
    const std::string& canvasId,
    const std::vector<DurationSegment>& segments,
    const std::vector<PitchPoint>& pitchPoints,
    int sampleRate
) {
    // JavaScript에서 Canvas element와 context 가져오기
    val document = val::global("document");
    val canvas = document.call<val>("getElementById", canvasId);

    if (canvas.isNull() || canvas.isUndefined()) {
        EM_ASM({ console.error('Canvas not found:', UTF8ToString($0)); }, canvasId.c_str());
        return;
    }

    val ctx = canvas.call<val>("getContext", std::string("2d"));
    int canvasWidth = canvas["width"].as<int>();
    int canvasHeight = canvas["height"].as<int>();

    // Canvas 초기화
    setFillStyle(ctx, "#1a1a1a");
    fillRect(ctx, 0, 0, canvasWidth, canvasHeight);

    if (segments.empty() && pitchPoints.empty()) {
        return;
    }

    // 전체 시간 범위 계산
    float maxTime = 0.0f;
    if (!segments.empty()) {
        maxTime = std::max(maxTime, segments.back().endTime);
    }
    if (!pitchPoints.empty()) {
        maxTime = std::max(maxTime, pitchPoints.back().time);
    }

    if (maxTime <= 0.0f) return;

    // 그래프 영역 설정 (여백 포함)
    float marginLeft = 60.0f;
    float marginRight = 20.0f;
    float marginTop = 40.0f;
    float marginBottom = 40.0f;
    float graphWidth = canvasWidth - marginLeft - marginRight;
    float graphHeight = canvasHeight - marginTop - marginBottom;

    // Duration 막대 그래프 그리기
    if (!segments.empty()) {
        float maxEnergy = 0.0f;
        for (const auto& seg : segments) {
            maxEnergy = std::max(maxEnergy, seg.energy);
        }

        for (const auto& seg : segments) {
            float x = marginLeft + (seg.startTime / maxTime) * graphWidth;
            float width = ((seg.endTime - seg.startTime) / maxTime) * graphWidth;

            // Energy에 따라 막대 높이 결정
            float barHeight = (seg.energy / maxEnergy) * graphHeight;
            float y = marginTop + (graphHeight - barHeight);

            // Energy에 따른 색상 (어두운 파랑 -> 밝은 파랑)
            float intensity = seg.energy / maxEnergy;
            int r = static_cast<int>(30 + intensity * 70);
            int g = static_cast<int>(60 + intensity * 140);
            int b = static_cast<int>(120 + intensity * 135);

            char color[32];
            snprintf(color, sizeof(color), "rgba(%d,%d,%d,0.6)", r, g, b);
            setFillStyle(ctx, color);
            fillRect(ctx, x, y, width, barHeight);

            // 막대 테두리
            setStrokeStyle(ctx, "rgba(100,150,200,0.8)");
            setLineWidth(ctx, 1.0f);
            strokeRect(ctx, x, y, width, barHeight);
        }
    }

    // Pitch 곡선 그리기
    if (!pitchPoints.empty()) {
        // Pitch 범위 계산
        float minPitch = 1000000.0f;
        float maxPitch = 0.0f;
        for (const auto& point : pitchPoints) {
            if (point.frequency > 0 && point.confidence > 0.5f) {
                minPitch = std::min(minPitch, point.frequency);
                maxPitch = std::max(maxPitch, point.frequency);
            }
        }

        // 유효한 pitch 범위 설정 (음성 주파수 범위: 80Hz ~ 500Hz)
        if (minPitch > maxPitch) {
            minPitch = 80.0f;
            maxPitch = 500.0f;
        } else {
            minPitch = std::max(50.0f, minPitch - 20.0f);
            maxPitch = std::min(600.0f, maxPitch + 20.0f);
        }

        // Pitch 곡선 그리기
        setStrokeStyle(ctx, "rgba(255,100,100,1.0)");
        setLineWidth(ctx, 3.0f);

        ctx.call<void>("beginPath");
        bool firstPoint = true;

        for (const auto& point : pitchPoints) {
            if (point.frequency > 0 && point.confidence > 0.5f) {
                float x = marginLeft + (point.time / maxTime) * graphWidth;
                float y = marginTop + (1.0f - (point.frequency - minPitch) / (maxPitch - minPitch)) * graphHeight;

                if (firstPoint) {
                    ctx.call<void>("moveTo", x, y);
                    firstPoint = false;
                } else {
                    ctx.call<void>("lineTo", x, y);
                }
            }
        }
        ctx.call<void>("stroke");

        // Pitch 포인트 표시
        setFillStyle(ctx, "rgba(255,100,100,0.8)");
        for (const auto& point : pitchPoints) {
            if (point.frequency > 0 && point.confidence > 0.7f) {
                float x = marginLeft + (point.time / maxTime) * graphWidth;
                float y = marginTop + (1.0f - (point.frequency - minPitch) / (maxPitch - minPitch)) * graphHeight;

                ctx.call<void>("beginPath");
                ctx.call<void>("arc", x, y, 4.0, 0.0, 6.28318530718);
                ctx.call<void>("fill");
            }
        }

        // Y축 레이블 (Pitch)
        setFillStyle(ctx, "#ffffff");
        setFont(ctx, "12px Arial");

        for (int i = 0; i <= 4; i++) {
            float freq = minPitch + (maxPitch - minPitch) * i / 4.0f;
            float y = marginTop + graphHeight - (graphHeight * i / 4.0f);

            char label[32];
            snprintf(label, sizeof(label), "%.0fHz", freq);
            fillText(ctx, label, 5, y + 4);
        }
    }

    // X축 레이블 (시간)
    setFillStyle(ctx, "#ffffff");
    setFont(ctx, "12px Arial");

    for (int i = 0; i <= 4; i++) {
        float time = maxTime * i / 4.0f;
        float x = marginLeft + (graphWidth * i / 4.0f);

        char label[32];
        snprintf(label, sizeof(label), "%.2fs", time);
        fillText(ctx, label, x - 20, canvasHeight - 10);
    }

    // 제목
    setFont(ctx, "bold 16px Arial");
    setFillStyle(ctx, "#ffffff");
    fillText(ctx, "Voice Analysis: Duration (bars) + Pitch (red curve)", canvasWidth / 2 - 180, 25);

    // 범례
    setFont(ctx, "12px Arial");

    // Duration 범례
    setFillStyle(ctx, "rgba(100,150,200,0.6)");
    fillRect(ctx, marginLeft, 5, 20, 15);
    setFillStyle(ctx, "#ffffff");
    fillText(ctx, "Duration/Energy", marginLeft + 25, 17);

    // Pitch 범례
    setStrokeStyle(ctx, "rgba(255,100,100,1.0)");
    setLineWidth(ctx, 3.0f);
    drawLine(ctx, marginLeft + 150, 12, marginLeft + 170, 12);
    setFillStyle(ctx, "#ffffff");
    fillText(ctx, "Pitch", marginLeft + 175, 17);
}

void CanvasRenderer::setFillStyle(val ctx, const std::string& color) {
    ctx.set("fillStyle", color);
}

void CanvasRenderer::setStrokeStyle(val ctx, const std::string& color) {
    ctx.set("strokeStyle", color);
}

void CanvasRenderer::setLineWidth(val ctx, float width) {
    ctx.set("lineWidth", width);
}

void CanvasRenderer::fillRect(val ctx, float x, float y, float width, float height) {
    ctx.call<void>("fillRect", x, y, width, height);
}

void CanvasRenderer::strokeRect(val ctx, float x, float y, float width, float height) {
    ctx.call<void>("strokeRect", x, y, width, height);
}

void CanvasRenderer::drawLine(val ctx, float x1, float y1, float x2, float y2) {
    ctx.call<void>("beginPath");
    ctx.call<void>("moveTo", x1, y1);
    ctx.call<void>("lineTo", x2, y2);
    ctx.call<void>("stroke");
}

void CanvasRenderer::fillText(val ctx, const std::string& text, float x, float y) {
    ctx.call<void>("fillText", text, x, y);
}

void CanvasRenderer::setFont(val ctx, const std::string& font) {
    ctx.set("font", font);
}

float CanvasRenderer::mapValue(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}
