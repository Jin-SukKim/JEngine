#pragma once
#include "pch.h"

namespace JEngine {
class Window
{
  public:
    Window(HINSTANCE hinstance, std::wstring name = L"JEngine");

    void Initialize();
    float AspectRatio() const;
    HWND GetHwnd() const;
    static Window* GetWindow() {
        return window_;
    }

    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  
    bool IsPaused() const {
        return appPaused_;
    }
    bool IsFullScreen() const {
        return fullScreenState_;
    }
    bool IsResizing() const {
        return resizing_;
    }
    bool IsMinimized() const {
        return minimized_;
    }
    bool IsMaximized() const {
        return maximized_;
    }
    
    UINT GetWidth() const {
        return screenWidth_;
    }
    UINT GetHeight() const {
        return screenHeight_;
    }
    float GetAspectRatio() const;

  private:
    static Window* window_;
    HINSTANCE appInst_ = nullptr;
    HWND mainWnd_ = nullptr;  // 윈도우 핸들
    UINT screenWidth_ = 1280; // 화면 너비
    UINT screenHeight_ = 720; // 화면 높이
    std::wstring mainWndCaption_;

    bool appPaused_ = false; // 애플리케이션 일시정지 상태
    bool minimized_ = false;
    bool maximized_ = false;
    bool resizing_ = false;
    bool fullScreenState_ = false;
};
} // namespace JEngine
