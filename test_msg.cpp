#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    MessageBoxW(nullptr, L"Test MinGW Subsystem", L"Success", MB_OK);
    return 0;
}
