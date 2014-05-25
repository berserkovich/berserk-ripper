#include "Ripper.h"

#include <memory>
#include <new>

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    RipperApp app(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    app.Initialize();
    return app.Run();
}
