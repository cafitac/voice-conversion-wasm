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

## 빌드 방법

1. 빌드 스크립트 실행 권한 부여:
   ```bash
   chmod +x build.sh
   ```

2. 빌드 실행:
   ```bash
   ./build.sh
   ```

## 실행 방법

빌드 후 웹 서버를 실행해야 합니다 (CORS 정책 때문에 file:// 프로토콜로는 작동하지 않습니다).

### 방법 1: Python 웹 서버
```bash
python3 -m http.server 8000
```

### 방법 2: Node.js serve
```bash
npx serve .
```

브라우저에서 `http://localhost:8000` 접속

## 파일 구조

- `main.cpp` - C++ 소스 코드
- `index.html` - 웹 페이지
- `build.sh` - 빌드 스크립트
- `main.js` - 생성된 JavaScript 글루 코드 (빌드 후)
- `main.wasm` - 생성된 WebAssembly 바이너리 (빌드 후)

## 코드 수정 후

코드를 수정한 후에는 다시 빌드해야 합니다:
```bash
./build.sh
```
