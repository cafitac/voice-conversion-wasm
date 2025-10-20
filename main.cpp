#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <string>

// C++ 함수: JavaScript에서 호출 가능
std::string greet(const std::string& name) {
    return "안녕하세요, " + name + "님!";
}

int add(int a, int b) {
    return a + b;
}

// Emscripten 바인딩
EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("greet", &greet);
    emscripten::function("add", &add);
}