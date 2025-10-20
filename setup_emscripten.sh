#!/bin/bash

EMSDK_DIR="./emsdk"

# Emscripten이 이미 설치되어 있는지 확인
if [ -d "$EMSDK_DIR" ]; then
    echo "Emscripten이 이미 설치되어 있습니다."
else
    echo "Emscripten을 설치합니다..."
    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk
    ./emsdk install latest
    ./emsdk activate latest
    cd ..
    echo "Emscripten 설치 완료!"
fi

echo ""
echo "설정이 완료되었습니다."
echo "이제 ./build.sh 를 실행하여 빌드할 수 있습니다."
