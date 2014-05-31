#include "Ripper.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    RipperApp app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    if (!app.Initialize())
    {
        MessageBox(NULL, L"Fatal error", L"Berserk Ripper", MB_ICONERROR | MB_OK);
        return 0;
    }
    return app.Run();
}
