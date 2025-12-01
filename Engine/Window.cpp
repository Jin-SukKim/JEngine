#include "pch.h"
#include "Window.h"

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return JEngine::Window::GetWindow()->MsgProc(hwnd, msg, wParam, lParam);
}

namespace JEngine {

Window* Window::window_ = nullptr;

Window::Window(HINSTANCE hinstance, std::wstring name)
    : appInst_(hinstance), mainWndCaption_(name) {
    // 1개의 Window 인스턴스만 존재하도록 설정
    if (window_ != nullptr) {
        ExitWithMessage("Main Window instance already exists!");
    }
    window_ = this;
    LogInfo("Main Window instance created.");
}

void Window::Initialize() {
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ::MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = appInst_;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWnd";

    if (!RegisterClass(&wc)) {
        ExitWithMessage("RegisterClass Failed.");
        return;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = {0, 0, static_cast<LONG>(screenWidth_), static_cast<LONG>(screenHeight_)};
    ::AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    mainWnd_ = ::CreateWindow(L"MainWnd", mainWndCaption_.c_str(), WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, appInst_, 0);
    if (!mainWnd_) {
        ExitWithMessage("CreateWindow Failed.");
        return;
    }

    ::ShowWindow(mainWnd_, SW_SHOW);
    ::UpdateWindow(mainWnd_);
}

float Window::AspectRatio() const {
    return static_cast<float>(screenWidth_) / screenHeight_;
}

HWND Window::GetHwnd() const {
    return mainWnd_;
}

LRESULT Window::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            appPaused_ = true;
            // mTimer.Stop();
        } else {
            appPaused_ = false;
            // mTimer.Start();
        }
        return 0;
    case WM_SIZE:
        screenWidth_ = LOWORD(lParam);
        screenHeight_ = HIWORD(lParam);

        if (wParam == SIZE_MINIMIZED) {
            appPaused_ = true;
            minimized_ = true;
            maximized_ = false;
        } else if (wParam == SIZE_MAXIMIZED) {
            appPaused_ = false;
            minimized_ = false;
            maximized_ = true;
            // OnResize();
        } else if (wParam == SIZE_RESTORED) {
            // Restoring from minimized state?
            if (minimized_) {
                appPaused_ = false;
                minimized_ = false;
                // OnResize();
            }
            // Restoring from maximized state?
            else if (maximized_) {
                appPaused_ = false;
                maximized_ = false;
                // OnResize();
            } else if (resizing_) {
                // Resizing by the user. Wait until the user is done resizing.
                // WM_ENTERSIZEMOVE와 WM_EXITSIZEMOVE에서 처리.
            } else {
                // OnResize();
            }
        }

        return 0;

    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
        appPaused_ = true;
        resizing_ = true;
        // timer_.Stop();
        return 0;

    // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
        appPaused_ = false;
        resizing_ = false;
        // timer_.Start();
        // OnResize();
        return 0;

    // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);

        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
float Window::GetAspectRatio() const {
    return static_cast<float>(screenWidth_) / static_cast<float>(screenHeight_);
}
} // namespace JEngine