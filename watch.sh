#!/bin/bash

# 파일 변경 감지 및 자동 빌드 스크립트
# 사용법: ./watch.sh

EMSDK_DIR="./emsdk"

# Emscripten이 설치되어 있는지 확인
if [ ! -d "$EMSDK_DIR" ]; then
    echo "Emscripten이 설치되어 있지 않습니다."
    exit 1
fi

# Python watchdog 설치 확인
if ! python3 -c "import watchdog" 2>/dev/null; then
    echo "watchdog 모듈이 설치되어 있지 않습니다."
    echo "설치 중..."
    pip3 install watchdog --user
fi

echo "파일 변경 감지 모드 시작..."
echo "감시 대상:"
echo "  - src/ (C++ 소스 파일)"
echo "  - web/js/ (JavaScript 파일)"
echo "  - web/css/ (CSS 파일)"
echo "  - web/*.html (HTML 파일)"
echo ""
echo "파일이 변경되면 자동으로 빌드합니다."
echo "종료하려면 Ctrl+C를 누르세요"
echo ""

# Python 스크립트로 파일 변경 감지 및 빌드
python3 << 'EOF'
import time
import subprocess
import sys
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class BuildHandler(FileSystemEventHandler):
    def __init__(self):
        self.last_build_time = 0
        self.debounce_seconds = 1  # 1초 디바운스
        
    def should_build(self, path):
        # 빌드 산출물은 제외
        if 'main.js' in path or 'main.wasm' in path:
            return False
        # .git, node_modules 등 제외
        if any(excluded in path for excluded in ['.git', 'node_modules', '__pycache__', '.DS_Store']):
            return False
        # C++ 소스 파일만 빌드 필요
        if any(ext in path for ext in ['.cpp', '.h']):
            return True
        return False
    
    def is_web_file(self, path):
        # 웹 파일 (JS, CSS, HTML) - 빌드 불필요, 브라우저 새로고침만
        if any(ext in path for ext in ['.js', '.css', '.html']):
            return True
        return False
    
    def on_modified(self, event):
        if event.is_directory:
            return
        
        path = event.src_path
        
        # 디바운스: 너무 자주 빌드하지 않도록
        current_time = time.time()
        if current_time - self.last_build_time < self.debounce_seconds:
            return
        
        # 웹 파일 (JS, CSS, HTML) 변경 시
        if self.is_web_file(path):
            print(f"\n[웹 파일 변경] {path}")
            print("→ 브라우저에서 새로고침하세요 (F5 또는 Cmd+R)")
            return
        
        # C++ 파일 변경 시 빌드
        if self.should_build(path):
            self.last_build_time = current_time
            print(f"\n[C++ 파일 변경] {path}")
            print("빌드 시작...")
            
            # 빌드 실행
            result = subprocess.run(['./build.sh'], capture_output=True, text=True)
            
            if result.returncode == 0:
                print("✓ 빌드 완료! 브라우저에서 새로고침하세요.")
            else:
                print("✗ 빌드 실패!")
                print(result.stderr)

if __name__ == "__main__":
    event_handler = BuildHandler()
    observer = Observer()
    
    # 감시할 디렉토리
    watch_dirs = ['src', 'web']
    
    for directory in watch_dirs:
        try:
            observer.schedule(event_handler, directory, recursive=True)
            print(f"감시 중: {directory}/")
        except Exception as e:
            print(f"경고: {directory} 디렉토리를 감시할 수 없습니다: {e}")
    
    observer.start()
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
        print("\n감시 종료")
    
    observer.join()
EOF

