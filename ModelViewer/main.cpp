#include "framework.h"
#include "ModelViewer.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    ModelViewer app;
    auto classname = L"MyApp";

    RunApplication* appRunner = new RunApplication(
        app, classname, hInstance, nCmdShow);

    appRunner->MessageLoop();
    appRunner->Terminate();

    delete appRunner;

    return 0;
}