#!/bin/bash

# 개발 모드: 파일 변경 감지 + 웹 서버
# 사용법: ./dev.sh

echo "개발 모드 시작..."
echo ""

# 초기 빌드
echo "초기 빌드 실행 중..."
./build.sh

if [ $? -ne 0 ]; then
    echo "초기 빌드 실패!"
    exit 1
fi

echo ""
echo "=========================================="
echo "개발 서버 시작"
echo "=========================================="
echo ""
echo "브라우저에서 다음 주소를 여세요:"
echo "  - 메인 앱: http://localhost:8088/web/"
echo ""
echo "파일 변경 감지:"
echo "  - C++ 파일 (.cpp, .h) 변경 → 자동 빌드"
echo "  - 웹 파일 (.js, .css, .html) 변경 → 브라우저 새로고침 필요"
echo ""
echo "종료하려면 Ctrl+C를 누르세요"
echo ""

# 웹 서버를 먼저 시작
python3 -m http.server 8088 &
SERVER_PID=$!

# 잠시 대기 (서버 시작 시간)
sleep 1

# 백그라운드에서 watch.sh 실행
./watch.sh &
WATCH_PID=$!

# 종료 시 정리
cleanup() {
    echo ""
    echo "서버 종료 중..."
    kill $WATCH_PID $SERVER_PID 2>/dev/null
    wait $WATCH_PID $SERVER_PID 2>/dev/null
    exit
}

trap cleanup INT TERM

# 프로세스 대기
wait

