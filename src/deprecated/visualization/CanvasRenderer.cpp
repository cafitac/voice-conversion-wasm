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
        // Step 1: 음성 구간 판별을 위한 energy threshold 계산
        std::vector<float> segmentEnergies;
        for (const auto& seg : segments) {
            segmentEnergies.push_back(seg.energy);
        }

        float voiceThreshold = 0.01f;
        if (!segmentEnergies.empty()) {
            std::sort(segmentEnergies.begin(), segmentEnergies.end());
            float median = segmentEnergies[segmentEnergies.size() / 2];
            voiceThreshold = std::max(0.01f, median * 0.2f);
        }

        // Step 2: 음성 구간의 유효한 pitch만 수집
        std::vector<float> validFrequencies;
        for (const auto& point : pitchPoints) {
            // Duration 세그먼트에서 해당 시간의 energy 확인
            bool isVoice = false;
            for (const auto& seg : segments) {
                if (point.time >= seg.startTime && point.time <= seg.endTime) {
                    if (seg.energy >= voiceThreshold) {
                        isVoice = true;
                    }
                    break;
                }
            }

            // 음성 구간의 유효한 주파수만 수집
            if (isVoice && point.frequency > 80.0f && point.frequency < 1000.0f) {
                validFrequencies.push_back(point.frequency);
            }
        }

        // Step 2: 중앙값 기반 이상치 제거
        float minPitch = 1000000.0f;
        float maxPitch = 0.0f;

        if (!validFrequencies.empty()) {
            // 중앙값 계산
            std::sort(validFrequencies.begin(), validFrequencies.end());
            float median = validFrequencies[validFrequencies.size() / 2];

            // IQR (Interquartile Range) 계산
            size_t q1Idx = validFrequencies.size() / 4;
            size_t q3Idx = (validFrequencies.size() * 3) / 4;
            float q1 = validFrequencies[q1Idx];
            float q3 = validFrequencies[q3Idx];
            float iqr = q3 - q1;

            // 이상치 범위: Q1 - 1.5*IQR ~ Q3 + 1.5*IQR
            float lowerBound = q1 - 1.5f * iqr;
            float upperBound = q3 + 1.5f * iqr;

            // 이상치를 제외한 범위 계산
            for (float freq : validFrequencies) {
                if (freq >= lowerBound && freq <= upperBound) {
                    minPitch = std::min(minPitch, freq);
                    maxPitch = std::max(maxPitch, freq);
                }
            }
        }

        // Step 3: 유효한 pitch 범위 동적 설정
        if (minPitch > maxPitch || validFrequencies.empty()) {
            // 데이터가 없으면 기본값
            minPitch = 80.0f;
            maxPitch = 500.0f;
        } else {
            // 실제 데이터 범위에서 여유 공간 추가
            float range = maxPitch - minPitch;
            float padding = std::max(50.0f, range * 0.2f); // 20% 또는 최소 50Hz

            minPitch = std::max(50.0f, minPitch - padding);
            maxPitch = std::min(1000.0f, maxPitch + padding); // 최대 1000Hz로 제한

            // 최소 범위 보장 (너무 좁은 범위 방지)
            if (maxPitch - minPitch < 200.0f) {
                float center = (minPitch + maxPitch) / 2.0f;
                minPitch = center - 100.0f;
                maxPitch = center + 100.0f;
            }
        }

        // Pitch 곡선 그리기 (모든 포인트, 음성 구간은 실제값, 비음성은 바닥)
        setStrokeStyle(ctx, "rgba(255,100,100,1.0)");
        setLineWidth(ctx, 3.0f);

        ctx.call<void>("beginPath");
        bool firstPoint = true;

        for (const auto& point : pitchPoints) {
            // Duration 세그먼트에서 해당 시간의 energy 확인
            bool isVoice = false;
            for (const auto& seg : segments) {
                if (point.time >= seg.startTime && point.time <= seg.endTime) {
                    if (seg.energy >= voiceThreshold) {
                        isVoice = true;
                    }
                    break;
                }
            }

            float x = marginLeft + (point.time / maxTime) * graphWidth;
            float y;

            if (isVoice && point.frequency >= minPitch && point.frequency <= maxPitch) {
                // 음성 구간: 실제 pitch 값 사용
                y = marginTop + (1.0f - (point.frequency - minPitch) / (maxPitch - minPitch)) * graphHeight;
            } else {
                // 비음성 구간: 바닥(minPitch)으로 표시
                y = marginTop + graphHeight;
            }

            if (firstPoint) {
                ctx.call<void>("moveTo", x, y);
                firstPoint = false;
            } else {
                ctx.call<void>("lineTo", x, y);
            }
        }
        ctx.call<void>("stroke");

        // Pitch 포인트 표시 (음성 구간만)
        setFillStyle(ctx, "rgba(255,100,100,0.8)");
        for (const auto& point : pitchPoints) {
            // Duration 세그먼트에서 해당 시간의 energy 확인
            bool isVoice = false;
            for (const auto& seg : segments) {
                if (point.time >= seg.startTime && point.time <= seg.endTime) {
                    if (seg.energy >= voiceThreshold) {
                        isVoice = true;
                    }
                    break;
                }
            }

            if (isVoice && point.frequency >= minPitch && point.frequency <= maxPitch) {
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

void CanvasRenderer::drawTrimHandles(
    const std::string& canvasId,
    float trimStart,
    float trimEnd,
    float maxTime
) {
    // JavaScript에서 Canvas element와 context 가져오기
    val document = val::global("document");
    val canvas = document.call<val>("getElementById", canvasId);

    if (canvas.isNull() || canvas.isUndefined()) {
        return;
    }

    val ctx = canvas.call<val>("getContext", std::string("2d"));
    int canvasWidth = canvas["width"].as<int>();
    int canvasHeight = canvas["height"].as<int>();

    // 그래프 영역 설정 (분석 그래프와 동일)
    float marginLeft = 60.0f;
    float marginRight = 20.0f;
    float graphWidth = canvasWidth - marginLeft - marginRight;

    // Trim 위치 계산
    float startX = marginLeft + (trimStart / maxTime) * graphWidth;
    float endX = marginLeft + (trimEnd / maxTime) * graphWidth;

    // 반투명 오버레이 (trim 되는 영역)
    setFillStyle(ctx, "rgba(0, 0, 0, 0.5)");
    fillRect(ctx, marginLeft, 0, startX - marginLeft, canvasHeight);
    fillRect(ctx, endX, 0, marginLeft + graphWidth - endX, canvasHeight);

    // Trim handles (노란색 막대)
    setFillStyle(ctx, "#FFD700");
    fillRect(ctx, startX - 3, 0, 6, canvasHeight);
    fillRect(ctx, endX - 3, 0, 6, canvasHeight);

    // 선택 영역 테두리
    setStrokeStyle(ctx, "#FFD700");
    setLineWidth(ctx, 2.0f);
    strokeRect(ctx, startX, 0, endX - startX, canvasHeight);
}
