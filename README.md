# C++ WebAssembly 프로젝트

C++로 작성한 코드를 WebAssembly로 컴파일하여 웹 브라우저에서 실행하는 프로젝트입니다.

## 필요 도구

1. **Emscripten SDK** 설치
   ```bash
   # Emscripten SDK 다운로드
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk

   # 최신 버전 설치
   ./emsdk install latest
   ./emsdk activate latest

   # 환경 변수 설정
   source ./emsdk_env.sh
   ```

## 빌드 및 실행 방법

1. 스크립트 실행 권한 부여:
   ```bash
   chmod +x runserver.sh
   ```

2. 서버 실행 (자동으로 빌드 후 서버 시작):
   ```bash
   ./runserver.sh
   ```

3. 브라우저에서 접속:
   ```
   http://localhost:8088/web/
   ```

`runserver.sh` 스크립트는 자동으로 빌드를 실행한 후 웹 서버를 시작합니다.

### 빌드만 실행하기

빌드만 따로 실행하려면:
```bash
./build.sh
```

## 파일 구조

- `main.cpp` - C++ 소스 코드
- `web/index.html` - 웹 페이지
- `build.sh` - 빌드 스크립트
- `runserver.sh` - 빌드 및 서버 실행 스크립트
- `web/main.js` - 생성된 JavaScript 글루 코드 (빌드 후)
- `web/main.wasm` - 생성된 WebAssembly 바이너리 (빌드 후)

## 코드 수정 후

코드를 수정한 후에는 서버를 재시작하세요 (Ctrl+C로 종료 후 다시 실행):
```bash
./runserver.sh
```
