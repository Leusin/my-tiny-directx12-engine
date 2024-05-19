#include "pch.h"
#include "GameCore.h"
#include "Display.h"
#include "Logger.h"

#include "SystemTime.h"

#include <io.h> // For Translation mode constants _O_TEXT (https://docs.microsoft.com/en-us/cpp/c-runtime-library/translation-mode-constants)
#include <fcntl.h>

gcore::RunApplication* g_app = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return g_app->GetWinMssageProc(hwnd, msg, wParam, lParam);
}

namespace gcore
{
RunApplication::RunApplication(IGameApp& app, const wchar_t* className, HINSTANCE hInst, int nCmdShow)
    : m_app(app)
    , m_graphics(std::make_unique<Graphics>())
{
    g_app = this;

#if defined(_DEBUG)
    // std::cout �� �ܼ�â ����
    CreateConsole();
#endif

    InitializeWindow(className, hInst);

    Initialize();

    ShowWindow(m_hWnd, nCmdShow /*SW_SHOWDEFAULT*/);
}

void RunApplication::InitializeWindow(const wchar_t* className, HINSTANCE hInst)
{
    RegisterWindowClass(hInst, className);

    display::ResolutionToUINT(display::k720p, m_width, m_hight);

    m_hWnd = CreateWindow(hInst, className, className, m_width, m_hight); // 1080p
}

void RunApplication::Initialize()
{
    // �ΰ� ���
    spdlog::stdout_color_mt(m_loggerName);

    spdlog::get(m_loggerName)->info("�ʱ�ȭ ����");

    SystemTime::Initialize();

    m_graphics->Initialize(m_hWnd, m_width, m_hight);

    // TODO-input �ʱ�ȭ

    m_app.Startup();

    spdlog::get(m_loggerName)->info("�ʱ�ȭ �Ϸ�");
}

void RunApplication::Terminate()
{
    m_app.Cleanup();

    m_graphics->Shutdown();
}

bool RunApplication::Update()
{
    // ��ŸŸ�� ��������
    static int64_t lastTick = SystemTime::GetCurrentTick();
    int64_t currentTick     = SystemTime::GetCurrentTick();
    double deltaTime        = SystemTime::TimeBetweenTicks(lastTick, currentTick);
    lastTick                = currentTick;

    m_app.Update(static_cast<float>(deltaTime));
    m_app.RenderScene();

    m_graphics->Update();
    m_graphics->Render();

    return !m_app.IsDone();
}

bool IGameApp::IsDone(void)
{
    return GetAsyncKeyState(VK_ESCAPE) & 0x8000;
}

LRESULT RunApplication::GetWinMssageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SIZE:
        {
            RECT clientRect = {};
            ::GetClientRect(m_hWnd, &clientRect);

            int width  = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            m_graphics->Resize(width, height);
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        case WM_KEYDOWN:
        {
            bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        }
    }

    return 0;
}

void RunApplication::MessageLoop()
{
    do
    {
        MSG msg   = {};
        bool done = false;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) done = true;
        }

        if (done) break;
    } while (Update()); // Returns false to quit loop
}

void RunApplication::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
    // ������ Ŭ���� ���
    WNDCLASSEX windowClass = {};

    windowClass.cbSize        = sizeof(WNDCLASSEX);
    windowClass.style         = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = WndProc;
    windowClass.cbClsExtra    = 0;
    windowClass.cbWndExtra    = 0;
    windowClass.hInstance     = hInst;
    windowClass.hIcon         = LoadIcon(hInst, IDI_APPLICATION);
    windowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName  = nullptr;
    windowClass.lpszClassName = windowClassName;
    windowClass.hIconSm       = LoadIcon(hInst, IDI_APPLICATION);

    static HRESULT hr = ::RegisterClassExW(&windowClass);
    assert(SUCCEEDED(hr));
}

HWND RunApplication::CreateWindow(HINSTANCE hInst,
    const wchar_t* windowClassName,
    const wchar_t* windowTitle,
    uint32_t width,
    uint32_t height)
{
    // GetSystemMetrics ������� ���� �ʺ�� ���̸� �����ɴϴ�.
    int screenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth  = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    HWND hWnd = ::CreateWindowExW(
        NULL, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInst, nullptr);

    assert(hWnd && "Failed to create window");

    return hWnd;
}


void RunApplication::CreateConsole()
{
    // �ܼ� �Ҵ�
    if (AllocConsole())
    {
        HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        // ȭ�� ���۸� �÷� ���� ���� �ؽ�Ʈ ���� ����� �� �ֵ��� �մϴ�.
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(lStdHandle, &consoleInfo);
        consoleInfo.dwSize.Y = MAX_CONSOLE_LINES;
        SetConsoleScreenBufferSize(lStdHandle, consoleInfo.dwSize);
        SetConsoleCursorPosition(lStdHandle, { 0, 0 });

        // ���۸����� ���� STDOUT�� �ֿܼ� ���𷺼��մϴ�.
        int hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        FILE* fp       = _fdopen(hConHandle, "w");
        freopen_s(&fp, "CONOUT$", "w", stdout);
        setvbuf(stdout, nullptr, _IONBF, 0);

        // ���۸����� ���� STDOUT�� �ֿܼ� ���𷺼��մϴ�.
        lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
        hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        fp         = _fdopen(hConHandle, "r");
        freopen_s(&fp, "CONIN$", "r", stdin);
        setvbuf(stdin, nullptr, _IONBF, 0);
        setvbuf(stdin, nullptr, _IONBF, 0);

        // ���۸����� ���� STDOUT�� �ֿܼ� ���𷺼��մϴ�.
        lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
        hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        fp         = _fdopen(hConHandle, "w");
        freopen_s(&fp, "CONOUT$", "w", stderr);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // �� C++ ǥ�� ��Ʈ�� ��ü�� ���� ���� ���¸� ����ϴ�.
        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
        std::wcin.clear();
        std::cin.clear();
    }
}

} // namespace gcore