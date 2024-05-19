#pragma once

#include "pch.h"
#include "Graphics.h"

namespace gcore
{
class IGameApp
{
public:
    // �� �Լ��� ���ø����̼��� �ʱ�ȭ�ϴ� �� ����� �� ������
    // �ʼ� �ϵ���� ���ҽ��� �Ҵ�� �Ŀ� ����˴ϴ�. �̷���
    // ���ҽ��� �������� �ʴ� �����ͳ� �÷��׿� ���� �ڷ�����
    // �����ڿ��� �ʱ�ȭ�ؾ� �մϴ�.
    virtual void Startup(void) = 0;
    virtual void Cleanup(void) = 0;

    // ���� �������� ���θ� �����մϴ�.
    // ���� 'ESC' Ű�� ���� ������ ��� ����˴ϴ�.
    virtual bool IsDone(void);

    // ������Ʈ �޼���� �����Ӵ� �� �� ȣ��˴ϴ�.
    // ������Ʈ ���װ� �� �������� �� �޼���� ó���ؾ� �մϴ�.
    virtual void Update(float deltaT) = 0;

    // Official rendering pass
    virtual void RenderScene(void) = 0;

    // LDR: ������ UI(��������) ������ �н�. ���۰� �̹� �� �����Դϴ�.
    virtual void RenderUI(class GraphicsContext&){};

    // ���̷�ƮX ����Ʈ���̽��� ����ϴ� ���� �� ���� �������Ͽ�
    // DXR ���� ����̽��� �䱸�մϴ�.
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