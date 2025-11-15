#!/bin/bash

echo "==========================================="
echo "Running All Audio Processing Benchmarks"
echo "==========================================="
echo ""

# 입력 파일 확인
INPUT_FILE="../original.wav"
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file not found: $INPUT_FILE"
    exit 1
fi

# 벤치마크 파라미터
SEMITONES=3.0
DURATION_RATIO=1.5

# 결과 디렉토리 생성
REPORT_DIR="../benchmark_result"
mkdir -p "$REPORT_DIR"

echo "Input File: $INPUT_FILE"
echo "Semitones: $SEMITONES"
echo "Duration Ratio: ${DURATION_RATIO}x"
echo ""

# 0. Partial Segment Benchmark
echo "[0/4] Running Partial Segment Benchmark..."
echo "========================================="
./benchmark_partialsegment "$INPUT_FILE" 2>&1 | grep -E "✓|✗|saved to|Total tests"
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✓ Partial Segment Benchmark completed"
else
    echo "✗ Partial Segment Benchmark failed"
fi
echo ""

# 1. Pitch Shift Benchmark
echo "[1/4] Running Pitch Shift Benchmark..."
echo "========================================="
./benchmark_pitchshift "$INPUT_FILE" "$SEMITONES" 2>&1 | grep -E "✓|✗|saved to"
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✓ Pitch Shift Benchmark completed"
else
    echo "✗ Pitch Shift Benchmark failed"
fi
echo ""

# 2. Time Stretch Benchmark
echo "[2/4] Running Time Stretch Benchmark..."
echo "========================================="
./benchmark_timestretch "$INPUT_FILE" "$DURATION_RATIO" 2>&1 | grep -E "✓|✗|saved to"
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✓ Time Stretch Benchmark completed"
else
    echo "✗ Time Stretch Benchmark failed"
fi
echo ""

# 3. Combined Benchmark
echo "[3/4] Running Combined Benchmark..."
echo "========================================="
./benchmark_combined "$INPUT_FILE" "$SEMITONES" "$DURATION_RATIO" 2>&1 | grep -E "✓|✗|saved to"
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "✓ Combined Benchmark completed"
else
    echo "✗ Combined Benchmark failed"
fi
echo ""

# 통합 보고서 생성
echo "==========================================="
echo "Generating Comprehensive Benchmark Report"
echo "==========================================="

# 탭 방식 통합 리포트 생성
bash create_tabbed_report.sh

FINAL_REPORT="$REPORT_DIR/comprehensive_benchmark_report.html"

echo ""
echo "==========================================="
echo "All Benchmarks Completed!"
echo "==========================================="
echo "Reports saved to: $REPORT_DIR/"
echo ""
echo "View reports:"
echo "  - Comprehensive: open $FINAL_REPORT"
echo "  - Partial Segment: open $REPORT_DIR/benchmark_partialsegment_report.html"
echo "  - Pitch Shift: open $REPORT_DIR/benchmark_pitchshift_report.html"
echo "  - Time Stretch: open $REPORT_DIR/benchmark_timestretch_report.html"
echo "  - Combined: open $REPORT_DIR/benchmark_combined_report.html"
echo ""
echo "Or visit: http://localhost:8088/web/benchmark/comprehensive_benchmark_report.html"
