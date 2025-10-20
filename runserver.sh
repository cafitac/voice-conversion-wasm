#!/bin/bash

# 먼저 빌드 실행
./build.sh

if [ $? -ne 0 ]; then
    echo "빌드 실패로 인해 서버를 시작할 수 없습니다."
    exit 1
fi

echo ""
echo "웹 서버를 시작합니다..."
echo "브라우저에서 http://localhost:8088/web/ 을 여세요"
echo ""
echo "종료하려면 Ctrl+C를 누르세요"
echo ""

python3 -m http.server 8088
