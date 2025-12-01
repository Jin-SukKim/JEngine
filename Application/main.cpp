#include <Windows.h>
#include "Engine/Application.h"

int main() {
    try {
        // 1. HINSTANCE 가져오기 (현재 프로세스의 인스턴스 핸들)
        HINSTANCE hInstance = GetModuleHandle(nullptr);

        // 2. Context 인스턴스 생성 (Singleton 패턴)
        JEngine::Application app(hInstance, L"JEngine");

        // 3. DirectX 12 및 윈도우 초기화
        app.Initialize();

        // 4. 메시지 루프 실행 (렌더링 루프)
        return app.Run();

    } catch (const std::exception& e) {
        // 예외 발생 시 에러 메시지 출력
        MessageBoxA(nullptr, e.what(), "Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}