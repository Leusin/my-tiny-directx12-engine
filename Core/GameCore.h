#pragma once

#include "pch.h"
#include "Graphics.h"

namespace gcore
{
class IGameApp
{
public:
    // 이 함수는 애플리케이션을 초기화하는 데 사용할 수 있으며
    // 필수 하드웨어 리소스가 할당된 후에 실행됩니다. 이러한
    // 리소스에 의존하지 않는 포인터나 플래그와 같은 자료형은
    // 생성자에서 초기화해야 합니다.
    virtual void Startup(void) = 0;
    virtual void Cleanup(void) = 0;

    // 앱을 종료할지 여부를 결정합니다.
    // 앱은 'ESC' 키를 누를 때까지 계속 실행됩니다.
    virtual bool IsDone(void);

    // 업데이트 메서드는 프레임당 한 번 호출됩니다.
    // 업데이트 사항과 씬 렌더링은 이 메서드로 처리해야 합니다.
    virtual void Update(float deltaT) = 0;

    // Official rendering pass
    virtual void RenderScene(void) = 0;

    // LDR: 선택적 UI(오버레이) 렌더링 패스. 버퍼가 이미 진 상태입니다.
    virtual void RenderUI(class GraphicsContext&){};

    // 다이렉트X 레이트레이싱을 사용하는 앱은 이 값을 재정의하여
    // DXR 지원 디바이스를 요구합니다.
    virtual bool RequiresRaytracingSupport() const
    {
        return false;
    }
};
} // namespace gcore

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace gcore
{
// int RunApplication(IGameApp& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow);

class RunApplication
{
public:
    RunApplication(IGameApp& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow);

    void MessageLoop();
    void Terminate();

    LRESULT CALLBACK GetWinMssageProc(HWND, UINT, WPARAM, LPARAM);

private:
    void InitializeWindow(const wchar_t* className, HINSTANCE hInst);
    void Initialize();
    bool Update();

    void CreateConsole();
    void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
    HWND CreateWindow(HINSTANCE hInst, const wchar_t* windowClassName, const wchar_t* windowTitle, uint32_t width, uint32_t height);

    IGameApp& m_app;
    bool m_isSupending          = false;
    const int MAX_CONSOLE_LINES = 500;

    HWND m_hWnd = nullptr;
    uint32_t m_width, m_hight;

    std::unique_ptr<Graphics> m_graphics;

    std::string m_loggerName = "runapp";
};
} // namespace gcore