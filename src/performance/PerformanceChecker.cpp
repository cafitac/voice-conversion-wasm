#include "PerformanceChecker.h"
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>

PerformanceChecker::PerformanceChecker() : totalDuration(0.0) {}

PerformanceChecker::~PerformanceChecker() {}

void PerformanceChecker::start(const std::string& label) {
    activeTimers[label] = std::chrono::high_resolution_clock::now();
}

double PerformanceChecker::end(const std::string& label) {
    auto endTime = std::chrono::high_resolution_clock::now();

    auto it = activeTimers.find(label);
    if (it == activeTimers.end()) {
        std::cerr << "Warning: Timer '" << label << "' was not started!" << std::endl;
        return 0.0;
    }

    auto startTime = it->second;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    double durationMs = duration.count() / 1000.0;

    // 결과 저장
    measurements[label].push_back(durationMs);

    // 활성 타이머에서 제거
    activeTimers.erase(it);

    return durationMs;
}

std::vector<double> PerformanceChecker::getMeasurements(const std::string& label) const {
    auto it = measurements.find(label);
    if (it != measurements.end()) {
        return it->second;
    }
    return std::vector<double>();
}

double PerformanceChecker::getAverage(const std::string& label) const {
    auto data = getMeasurements(label);
    if (data.empty()) {
        return 0.0;
    }
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

void PerformanceChecker::reset() {
    activeTimers.clear();
    measurements.clear();
    currentFeature.reset();
    functionStack.clear();
    completedFeatures.clear();
    totalDuration = 0.0;
}

// === 계층적 측정 API 구현 ===

void PerformanceChecker::startFeature(const std::string& name) {
    currentFeature = std::make_unique<FeatureContext>();
    currentFeature->name = name;
    currentFeature->startTime = std::chrono::high_resolution_clock::now();
}

void PerformanceChecker::endFeature() {
    if (!currentFeature) {
        std::cerr << "Warning: No feature to end!" << std::endl;
        return;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - currentFeature->startTime
    );
    double durationMs = duration.count() / 1000.0;

    FeatureNode feature;
    feature.feature = currentFeature->name;
    feature.duration = durationMs;
    feature.functions = currentFeature->functions;

    completedFeatures.push_back(feature);
    totalDuration += durationMs;

    currentFeature.reset();
    functionStack.clear();
}

void PerformanceChecker::startFunction(const std::string& name) {
    FunctionContext func;
    func.name = name;
    func.startTime = std::chrono::high_resolution_clock::now();
    functionStack.push_back(func);
}

void PerformanceChecker::endFunction() {
    if (functionStack.empty()) {
        std::cerr << "Warning: No function to end!" << std::endl;
        return;
    }

    auto& func = functionStack.back();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - func.startTime
    );
    double durationMs = duration.count() / 1000.0;

    FunctionNode node;
    node.name = func.name;
    node.duration = durationMs;
    node.children = func.children;

    functionStack.pop_back();

    // 부모 함수가 있으면 자식으로 추가, 없으면 기능의 함수로 추가
    if (!functionStack.empty()) {
        functionStack.back().children.push_back(node);
    } else if (currentFeature) {
        currentFeature->functions.push_back(node);
    }
}

std::vector<PerformanceChecker::FeatureNode> PerformanceChecker::getFeatures() const {
    return completedFeatures;
}

double PerformanceChecker::getTotalDuration() const {
    return totalDuration;
}

// 재귀적으로 FunctionNode를 JSON으로 직렬화하는 헬퍼 함수
static void serializeFunctionNode(std::ostringstream& oss, const PerformanceChecker::FunctionNode& func, int indent) {
    std::string indentStr(indent, ' ');

    oss << "\n" << indentStr << "{\n";
    oss << indentStr << "  \"name\": \"" << func.name << "\",\n";
    oss << indentStr << "  \"duration\": " << std::fixed << std::setprecision(3) << func.duration;

    // 자식 함수들이 있으면 재귀적으로 출력
    if (!func.children.empty()) {
        oss << ",\n" << indentStr << "  \"children\": [";

        bool firstChild = true;
        for (const auto& child : func.children) {
            if (!firstChild) oss << ",";
            firstChild = false;
            serializeFunctionNode(oss, child, indent + 4);
        }

        oss << "\n" << indentStr << "  ]";
    }

    oss << "\n" << indentStr << "}";
}

std::string PerformanceChecker::getReportJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"totalDuration\": " << std::fixed << std::setprecision(3) << totalDuration << ",\n";
    oss << "  \"features\": [";

    // 계층적 구조 출력
    bool firstFeature = true;
    for (const auto& feature : completedFeatures) {
        if (!firstFeature) oss << ",";
        firstFeature = false;

        oss << "\n    {\n";
        oss << "      \"feature\": \"" << feature.feature << "\",\n";
        oss << "      \"duration\": " << std::fixed << std::setprecision(3) << feature.duration << ",\n";
        oss << "      \"functions\": [";

        bool firstFunc = true;
        for (const auto& func : feature.functions) {
            if (!firstFunc) oss << ",";
            firstFunc = false;
            serializeFunctionNode(oss, func, 8);
        }

        oss << "\n      ]\n";
        oss << "    }";
    }

    oss << "\n  ],\n";
    oss << "  \"measurements\": {\n";

    // 평면 측정 데이터도 포함 (하위 호환성)
    bool first = true;
    for (const auto& pair : measurements) {
        if (!first) {
            oss << ",\n";
        }
        first = false;

        const std::string& label = pair.first;
        const std::vector<double>& durations = pair.second;

        double avg = getAverage(label);
        double min = *std::min_element(durations.begin(), durations.end());
        double max = *std::max_element(durations.begin(), durations.end());

        oss << "    \"" << label << "\": {\n";
        oss << "      \"count\": " << durations.size() << ",\n";
        oss << "      \"average\": " << std::fixed << std::setprecision(3) << avg << ",\n";
        oss << "      \"min\": " << std::fixed << std::setprecision(3) << min << ",\n";
        oss << "      \"max\": " << std::fixed << std::setprecision(3) << max << "\n";
        oss << "    }";
    }

    oss << "\n  }\n";
    oss << "}";
    return oss.str();
}

std::string PerformanceChecker::getReportCSV() const {
    std::ostringstream oss;
    oss << "Label,Count,Average(ms),Min(ms),Max(ms)\n";

    for (const auto& pair : measurements) {
        const std::string& label = pair.first;
        const std::vector<double>& durations = pair.second;

        double avg = getAverage(label);
        double min = *std::min_element(durations.begin(), durations.end());
        double max = *std::max_element(durations.begin(), durations.end());

        oss << label << ","
            << durations.size() << ","
            << std::fixed << std::setprecision(3) << avg << ","
            << std::fixed << std::setprecision(3) << min << ","
            << std::fixed << std::setprecision(3) << max << "\n";
    }

    return oss.str();
}
