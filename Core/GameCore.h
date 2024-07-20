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
    /// �� �Լ��� ���ø����̼��� �ʱ�ȭ�ϴ� �� ����� �� ������
    /// �ʼ� �ϵ���� ���ҽ��� �Ҵ�� �Ŀ� ����˴ϴ�. �̷���
    /// ���ҽ��� �������� �ʴ� �����ͳ� �÷��׿� ���� �ڷ�����
    /// �����ڿ��� �ʱ�ȭ�ؾ� �մϴ�.
    /// </summary>
    virtual void Startup(void) abstract;

    /// <summary>
    /// �������� �ε��մϴ�.
    /// </summary>
    virtual void LoadContent() abstract;

    /// <summary>
    /// �ε��� �������� ��ε� �մϴ�.
    /// </summary>
    virtual void UnloadContent() abstract;

    /// <summary>
    /// ���ӿ��� �� ��� ���ҽ��� �����մϴ�.
    /// </summary>
    virtual void Cleanup(void) abstract;

    /// <summary>
    /// ���� �������� ���θ� �����մϴ�.
    /// </summary>
    bool IsDone(void);
    void ToggleIsDone();

    /// <summary>
    /// ���� ������ ������Ʈ �մϴ�.
    /// ������Ʈ �޼���� �����Ӵ� �� �� ȣ��˴ϴ�.
    /// </summary>
    virtual void Update(UpdateEventArgs& e);

    /// <summary>
    /// �ֵ� ���� pass �Դϴ�.
    /// </summary>
    /// <param name="e"></param>
    virtual void Render(RenderEventArgs& e);

    /// <summary>
    /// �����찡 ��Ŀ�� �Ǵ� ����
    /// Ű�� ������ �� ��ϵ� �����쿡 ���� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnKeyPressed(KeyEventArgs& e);

    /// <summary>
    /// Ű�� Ű���忡�� ����� �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnKeyReleased(KeyEventArgs& e);

    /// <summary>
    /// ���콺�� ��ϵ� �����쿡�� ������ �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnMouseMoved(MouseMotionEventArgs& e);

    /// <summary>
    /// ���콺 ��ư�� ��ϵ� �����쿡�� ������ �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);

    /// <summary>
    /// ���콺 ��ư�� ��ϵ� �����쿡�� �������� �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);

    /// <summary>
    /// ���콺 ���� ��ϵ� �����쿡�� ��ũ�� �� �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnMouseWheel(MouseWheelEventArgs& e);

    /// <summary>
    /// �����찡 ��������� �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnResize(ResizeEventArgs& e);

    /// <summary>
    /// ��ϵ� ������ �ν��Ͻ��� �ı��� �� ȣ��˴ϴ�.
    /// </summary>
    virtual void OnWindowDestroy();

    // ���̷�ƮX ����Ʈ���̽��� ����ϴ� ���� �� ���� �������Ͽ�
    // DXR ���� ����̽��� �䱸�մϴ�.
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
    /// ��ü ȭ�� â���� ��ȯ�� �� ���� â ũ�⸦ ������ �ݴϴ�.
    /// </summary>
    RECT m_windowRect;

    std::unique_ptr<Graphics> m_graphics;

    std::string m_loggerName = "runapp";

    const int m_minWidth = 200;
    const int m_minHight = 150;
};

} // namespace gcore