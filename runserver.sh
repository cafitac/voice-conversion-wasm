#!/bin/bash

# 먼저 빌드 실행
./build.sh

if [ $? -ne 0 ]; then
    echo "빌드 실패로 인해 서버를 시작할 수 없습니다."
    exit 1
fi

echo ""
echo "벤치마킹 실행 중..."
echo "=========================================="
cd tests

# 모든 벤치마크 빌드
./build_all_benchmarks.sh > /dev/null 2>&1

# 벤치마크 실행
./run_all_benchmarks.sh > /dev/null 2>&1

cd ..
echo "✓ 벤치마킹 완료"
echo ""

# benchmark_result를 web 디렉토리에 심볼릭 링크 생성
if [ ! -L "web/benchmark" ]; then
    ln -sf ../benchmark_result web/benchmark
    echo "✓ 벤치마크 결과를 web/benchmark 에 링크했습니다"
fi

echo ""
echo "웹 서버를 시작합니다..."
echo "브라우저에서 다음 주소를 여세요:"
echo "  - 메인 앱: http://localhost:8088/web/"
echo "  - 벤치마크 리포트: http://localhost:8088/web/benchmark/comprehensive_benchmark_report.html"
echo ""
echo "종료하려면 Ctrl+C를 누르세요"
echo ""

python3 -m http.server 8088
