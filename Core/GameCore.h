#pragma once

#include "pch.h"
#include "Graphics.h"
#include "Events.h"

#include "HighResolutionClock.h"

namespace gcore
{
class IGameApp: public std::enable_shared_from_this<IGameApp>
{
public:
    IGameApp()          = default;
    virtual ~IGameApp() = default;

    /// <summary>
    /// 이 함수는 애플리케이션을 초기화하는 데 사용할 수 있으며
    /// 필수 하드웨어 리소스가 할당된 후에 실행됩니다. 이러한
    /// 리소스에 의존하지 않는 포인터나 플래그와 같은 자료형은
    /// 생성자에서 초기화해야 합니다.
    /// </summary>
    virtual void Startup(void) abstract;

    /// <summary>
    /// 콘텐츠를 로드합니다.
    /// </summary>
    virtual void LoadContent() abstract;

    /// <summary>
    /// 로드한 콘텐츠를 언로드 합니다.
    /// </summary>
    virtual void UnloadContent() abstract;

    /// <summary>
    /// 게임에서 다 모두 리소스를 제거합니다.
    /// </summary>
    virtual void Cleanup(void) abstract;

    /// <summary>
    /// 앱을 종료할지 여부를 결정합니다.
    /// </summary>
    bool IsDone(void);
    void ToggleIsDone();

    /// <summary>
    /// 게임 로직을 업데이트 합니다.
    /// 업데이트 메서드는 프레임당 한 번 호출됩니다.
    /// </summary>
    virtual void Update(UpdateEventArgs& e);

    /// <summary>
    /// 주된 랜더 pass 입니다.
    /// </summary>
    /// <param name="e"></param>
    virtual void Render(RenderEventArgs& e);

    /// <summary>
    /// 윈도우가 포커스 되는 동안
    /// 키가 눌렸을 때 등록된 윈도우에 의해 호출됩니다.
    /// </summary>
    virtual void OnKeyPressed(KeyEventArgs& e);

    /// <summary>
    /// 키가 키보드에서 뗴어질 떄 호출됩니다.
    /// </summary>
    virtual void OnKeyReleased(KeyEventArgs& e);

    /// <summary>
    /// 마우스가 등록된 윈도우에서 움직일 떄 호출됩니다.
    /// </summary>
    virtual void OnMouseMoved(MouseMotionEventArgs& e);

    /// <summary>
    /// 마우스 버튼이 등록된 윈도우에서 눌렸을 때 호출됩니다.
    /// </summary>
    virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);

    /// <summary>
    /// 마우스 버튼이 등록된 윈도우에서 떼어졌을 떄 호출됩니다.
    /// </summary>
    virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);

    /// <summary>
    /// 마우스 휠이 등록된 윈도우에서 스크롤 될 때 호출됩니다.
    /// </summary>
    virtual void OnMouseWheel(MouseWheelEventArgs& e);

    /// <summary>
    /// 윈도우가 리사이즈될 떄 호출됩니다.
    /// </summary>
    virtual void OnResize(ResizeEventArgs& e);

    /// <summary>
    /// 등록된 윈도우 인스턴스가 파괴될 떄 호출됩니다.
    /// </summary>
    virtual void OnWindowDestroy();

    // 다이렉트X 레이트레이싱을 사용하는 앱은 이 값을 재정의하여
    // DXR 지원 디바이스를 요구합니다.
    virtual bool RequiresRaytracingSupport() const
    {
        return false;
    }

    int GetClientWidth() const;

    int GetClientHeight() const;

    void ToggleFullscreen();

    Graphics& GetGraphics() const;
    void SetGraphics(Graphics* graphics);

private:
    bool m_isDone;

    int m_width;
    int m_height;
    bool m_vSync;

    bool m_fullscreen;
    Graphics* m_graphics;
};
} // namespace gcore

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace gcore
{
class RunApplication
{
public:
    RunApplication(IGameApp& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow);
    ~RunApplication();

    void MessageLoop();
    void Terminate();

    LRESULT CALLBACK GetWinMssageProc(HWND, UINT, WPARAM, LPARAM);

    void Flush();

    void SetFullscreen(bool fullscreen);

    static uint64_t GetFrameCount();

private:
    void InitializeWindow(const wchar_t* className, HINSTANCE hInst);
    void Initialize();
    bool Update();

    void CreateConsole();
    void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName);
    HWND CreateWindow(HINSTANCE hInst, const wchar_t* windowClassName, const wchar_t* windowTitle, uint32_t width, uint32_t height);

    HighResolutionClock m_UpdateClock;
    HighResolutionClock m_RenderClock;
    static uint64_t ms_FrameCounter;

    IGameApp& m_app;
    bool m_isSupending;
    bool m_fullscreen;
    const int MAX_CONSOLE_LINES = 500;

    HWND m_hWnd = nullptr;
    uint32_t m_width, m_hight;

    /// <summary>
    /// 전체 화면 창으로 전환할 때 이전 창 크기를 저장해 뒵니다.
    /// </summary>
    RECT m_windowRect;

    std::unique_ptr<Graphics> m_graphics;

    std::string m_loggerName = "runapp";

    const int m_minWidth = 200;
    const int m_minHight = 150;
};

} // namespace gcore