#include "pch.h"
#include "GameCore.h"
#include "Display.h"
#include "Logger.h"

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
    // std::cout 로 콘솔창 생성
    CreateConsole();

    // debug 로 전역 로그 래벨 설정
    spdlog::set_level(spdlog::level::debug);
#endif

    InitializeWindow(className, hInst);

    Initialize();

    ShowWindow(m_hWnd, nCmdShow /*SW_SHOWDEFAULT*/);
}

RunApplication::~RunApplication()
{
    Flush();
}

void RunApplication::InitializeWindow(const wchar_t* className, HINSTANCE hInst)
{
    RegisterWindowClass(hInst, className);

    display::ResolutionToUINT(display::k720p, m_width, m_hight);

    m_hWnd = CreateWindow(hInst, className, className, m_width, m_hight); // 1080p
}

void RunApplication::Initialize()
{
    // 로거 등록
    spdlog::stdout_color_mt(m_loggerName);
    spdlog::get(m_loggerName)->info("초기화 시작...");

    m_graphics->Initialize(m_hWnd, m_width, m_hight);
    m_app.SetGraphics(m_graphics.get());

    spdlog::get(m_loggerName)->info("그래픽스 초기화 완료..");

    m_app.Startup();
    m_app.LoadContent();

    spdlog::get(m_loggerName)->info("응용프로그램 초기화 완료..");
}

void RunApplication::Terminate()
{
    Flush();

    m_app.UnloadContent();
    m_app.Cleanup();

    m_graphics->Shutdown();
}

bool RunApplication::Update()
{
    // OnUpdate
    m_UpdateClock.Tick();
    ms_FrameCounter++;

    UpdateEventArgs updateEventArgs(m_UpdateClock.GetDeltaSeconds(), m_UpdateClock.GetTotalSeconds());
    m_app.Update(updateEventArgs);

    // OnRender
    m_RenderClock.Tick();
    RenderEventArgs renderEventArgs(m_RenderClock.GetDeltaSeconds(), m_RenderClock.GetTotalSeconds());
    m_app.Render(renderEventArgs);

    return !m_app.IsDone();
}

bool IGameApp::IsDone(void)
{
    return m_isDone;
}

void IGameApp::ToggleIsDone()
{
    m_isDone = m_isDone ? false : true;
}

void IGameApp::Update(UpdateEventArgs& e)
{
}

void IGameApp::Render(RenderEventArgs& e)
{
}

void IGameApp::OnKeyPressed(KeyEventArgs& e)
{
}

void IGameApp::OnKeyReleased(KeyEventArgs& e)
{
}

void IGameApp::OnMouseMoved(MouseMotionEventArgs& e)
{
}

void IGameApp::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
}

void IGameApp::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
}

void IGameApp::OnMouseWheel(MouseWheelEventArgs& e)
{
}

int IGameApp::GetClientWidth() const
{
    return m_width;
}

int IGameApp::GetClientHeight() const
{
    return m_height;
}

void IGameApp::OnResize(ResizeEventArgs& e)
{
    m_width  = e.Width;
    m_height = e.Height;
}

void IGameApp::OnWindowDestroy()
{
    UnloadContent();
}

void IGameApp::ToggleFullscreen()
{
    g_app->SetFullscreen(m_fullscreen);
    m_fullscreen = m_fullscreen ? false : true;
}

Graphics& IGameApp::GetGraphics() const
{
    return *m_graphics;
}

void IGameApp::SetGraphics(Graphics* graphics)
{
    m_graphics = graphics;
}

MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch (messageID)
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        {
            mouseButton = MouseButtonEventArgs::Left;
        }
        break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        {
            mouseButton = MouseButtonEventArgs::Right;
        }
        break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
        {
            mouseButton = MouseButtonEventArgs::Middel;
        }
        break;
    }

    return mouseButton;
}

uint64_t RunApplication::ms_FrameCounter = 0; // 정적 멤버 변수 정의 및 초기화

LRESULT RunApplication::GetWinMssageProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            MSG charMsg;
            // Get the Unicode character (UTF-16)
            unsigned int c = 0;
            // For printable characters, the next message will be WM_CHAR.
            // This message contains the character code we need to send the KeyPressed event.
            // Inspired by the SDL 1.2 implementation.
            if (PeekMessage(&charMsg, hWnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
            {
                GetMessage(&charMsg, hWnd, 0, 0);
                c = static_cast<unsigned int>(charMsg.wParam);
            }
            bool shift            = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control          = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt              = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key      = (KeyCode::Key)wParam;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
            m_app.OnKeyPressed(keyEventArgs);
        }
        break;
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            bool shift            = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            bool control          = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt              = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            KeyCode::Key key      = (KeyCode::Key)wParam;
            unsigned int c        = 0;
            unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

            // Determine which key was released by converting the key code and the scan code
            // to a printable character (if possible).
            // Inspired by the SDL 1.2 implementation.
            unsigned char keyboardState[256];
            auto getkeystate = GetKeyboardState(keyboardState);
            wchar_t translatedCharacters[4];
            if (int result = ToUnicodeEx(
                                 static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
            {
                c = translatedCharacters[0];
            }

            KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
            m_app.OnKeyReleased(keyEventArgs);
        }
        break;
        // The default window procedure will play a system notification sound
        // when pressing the Alt+Enter keyboard combination if this message is
        // not handled.
        case WM_SYSCHAR:
            break;
        case WM_MOUSEMOVE:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift   = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
            m_app.OnMouseMoved(mouseMotionEventArgs);
        }
        break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift   = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs(
                DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
            m_app.OnMouseButtonPressed(mouseButtonEventArgs);
        }
        break;
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        {
            bool lButton = (wParam & MK_LBUTTON) != 0;
            bool rButton = (wParam & MK_RBUTTON) != 0;
            bool mButton = (wParam & MK_MBUTTON) != 0;
            bool shift   = (wParam & MK_SHIFT) != 0;
            bool control = (wParam & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            MouseButtonEventArgs mouseButtonEventArgs(
                DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
            m_app.OnMouseButtonReleased(mouseButtonEventArgs);
        }
        break;
        case WM_MOUSEWHEEL:
        {
            // The distance the mouse wheel is rotated.
            // A positive value indicates the wheel was rotated to the right.
            // A negative value indicates the wheel was rotated to the left.
            float zDelta    = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
            short keyStates = (short)LOWORD(wParam);

            bool lButton = (keyStates & MK_LBUTTON) != 0;
            bool rButton = (keyStates & MK_RBUTTON) != 0;
            bool mButton = (keyStates & MK_MBUTTON) != 0;
            bool shift   = (keyStates & MK_SHIFT) != 0;
            bool control = (keyStates & MK_CONTROL) != 0;

            int x = ((int)(short)LOWORD(lParam));
            int y = ((int)(short)HIWORD(lParam));

            // Convert the screen coordinates to client coordinates.
            POINT clientToScreenPoint;
            clientToScreenPoint.x = x;
            clientToScreenPoint.y = y;
            ScreenToClient(hWnd, &clientToScreenPoint);

            MouseWheelEventArgs mouseWheelEventArgs(
                zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
            m_app.OnMouseWheel(mouseWheelEventArgs);
        }
        break;
        case WM_SIZE:
        {
            int width  = ((int)(short)LOWORD(lParam));
            int height = ((int)(short)HIWORD(lParam));


            ResizeEventArgs resizeEventArgs(width, height);
            m_graphics->Resize(width, height);
            m_app.OnResize(resizeEventArgs);
        }
        break;
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;

            // 최소 트랙 크기를 설정합니다.
            pMinMaxInfo->ptMinTrackSize.x = m_minWidth;
            pMinMaxInfo->ptMinTrackSize.y = m_minHight;
            return 0; // 메시지가 처리되었음을 알림
        }
        case WM_CLOSE:
        {
            m_app.ToggleIsDone();
        }
        break;
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

void RunApplication::Flush()
{
    m_graphics->Flush();
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

            if (msg.message == WM_QUIT)
            {
                done = true;
            }
        }

        if (done) break;
    } while (Update()); // Returns false to quit loop
}

void RunApplication::RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
    // 윈도우 클래스 등록
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
    // GetSystemMetrics 모니터의 실제 너비와 높이를 가져옵니다.
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

void RunApplication::SetFullscreen(bool fullscreen)
{
    if (m_fullscreen != fullscreen)
    {
        m_fullscreen = fullscreen;

        if (m_fullscreen) // Switching to fullscreen.
        {
            // Store the current window dimensions so they can be restored
            // when switching out of fullscreen state.
            ::GetWindowRect(m_hWnd, &m_windowRect);

            // Set the window style to a borderless window so the client area fills
            // the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW &
                               ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

            // Query the name of the nearest display device for the window.
            // This is required to set the fullscreen dimensions of the window
            // when using a multi-monitor setup.
            HMONITOR hMonitor         = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize        = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);

            ::SetWindowPos(m_hWnd,
                HWND_TOPMOST,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_hWnd, SW_MAXIMIZE);
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(m_hWnd,
                HWND_NOTOPMOST,
                m_windowRect.left,
                m_windowRect.top,
                m_windowRect.right - m_windowRect.left,
                m_windowRect.bottom - m_windowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_hWnd, SW_NORMAL);
        }
    }
}

uint64_t RunApplication::GetFrameCount()
{
    return ms_FrameCounter;
}

void RunApplication::CreateConsole()
{
    // 콘솔 할당
    if (AllocConsole())
    {
        HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);

        // 화면 버퍼를 늘려 보다 많은 텍스트 줄을 사용할 수 있도록 합니다.
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(lStdHandle, &consoleInfo);
        consoleInfo.dwSize.Y = MAX_CONSOLE_LINES;
        SetConsoleScreenBufferSize(lStdHandle, consoleInfo.dwSize);
        SetConsoleCursorPosition(lStdHandle, { 0, 0 });

        // 버퍼링되지 않은 STDOUT을 콘솔에 리디렉션합니다.
        int hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        FILE* fp       = _fdopen(hConHandle, "w");
        freopen_s(&fp, "CONOUT$", "w", stdout);
        setvbuf(stdout, nullptr, _IONBF, 0);

        // 버퍼링되지 않은 STDOUT을 콘솔에 리디렉션합니다.
        lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
        hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        fp         = _fdopen(hConHandle, "r");
        freopen_s(&fp, "CONIN$", "r", stdin);
        setvbuf(stdin, nullptr, _IONBF, 0);
        setvbuf(stdin, nullptr, _IONBF, 0);

        // 버퍼링되지 않은 STDOUT을 콘솔에 리디렉션합니다.
        lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
        hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
        fp         = _fdopen(hConHandle, "w");
        freopen_s(&fp, "CONOUT$", "w", stderr);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // 각 C++ 표준 스트림 객체에 대한 오류 상태를 지웁니다.
        std::wcout.clear();
        std::cout.clear();
        std::wcerr.clear();
        std::cerr.clear();
        std::wcin.clear();
        std::cin.clear();
    }
}

} // namespace gcore