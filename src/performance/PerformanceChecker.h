#ifndef PERFORMANCE_CHECKER_H
#define PERFORMANCE_CHECKER_H

#include <string>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <memory>

/**
 * PerformanceChecker - 성능 측정 유틸리티
 * 각 기능의 실행 시간을 측정하고 결과를 수집
 * 계층적 구조 지원 추가
 */
class PerformanceChecker {
public:
    // 계층적 함수 정보
    struct FunctionNode {
        std::string name;
        double duration;
        std::vector<FunctionNode> children;
    };

    // 기능 정보
    struct FeatureNode {
        std::string feature;
        double duration;
        std::vector<FunctionNode> functions;
    };

    struct Measurement {
        std::string label;
        double durationMs;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    };

    PerformanceChecker();
    ~PerformanceChecker();

    // 측정 시작
    void start(const std::string& label);

    // 측정 종료 및 기록
    double end(const std::string& label);

    // 특정 레이블의 모든 측정 결과 조회
    std::vector<double> getMeasurements(const std::string& label) const;

    // 특정 레이블의 평균 실행 시간
    double getAverage(const std::string& label) const;

    // 모든 측정 결과 초기화
    void reset();

    // 결과를 JSON 문자열로 반환
    std::string getReportJSON() const;

    // 결과를 CSV 문자열로 반환
    std::string getReportCSV() const;

    // === 계층적 측정 API ===
    void startFeature(const std::string& name);
    void endFeature();
    void startFunction(const std::string& name);
    void endFunction();
    std::vector<FeatureNode> getFeatures() const;
    double getTotalDuration() const;

private:
    // 현재 실행 중인 측정들 (label -> start time)
    std::unordered_map<std::string, std::chrono::time_point<std::chrono::high_resolution_clock>> activeTimers;

    // 완료된 측정 결과들 (label -> list of durations)
    std::unordered_map<std::string, std::vector<double>> measurements;

    // 계층적 측정용
    struct FeatureContext {
        std::string name;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::vector<FunctionNode> functions;
    };

    struct FunctionContext {
        std::string name;
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::vector<FunctionNode> children;
    };

    std::unique_ptr<FeatureContext> currentFeature;
    std::vector<FunctionContext> functionStack;
    std::vector<FeatureNode> completedFeatures;
    double totalDuration;
};

#endif // PERFORMANCE_CHECKER_H
