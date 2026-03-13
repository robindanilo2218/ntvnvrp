// ============================================================
// NativeNav - Lightweight Browser for Windows
// main.cpp - Application entry point
// ============================================================
//
// NativeNav es un navegador web ligero para Windows que incluye:
// - Soporte completo HTML5/CSS3/JavaScript via WebView2
// - Bloqueador de anuncios integrado
// - Control de cookies (bloqueo de terceros)
// - Auto-eliminación de banners de cookies y popups molestos
// - Múltiples motores de búsqueda configurables
// - Configuración editable via archivos JSON
//
// Compilar con MSVC:
//   cl /EHsc /std:c++17 /DUNICODE /D_UNICODE main.cpp browser.cpp 
//      /I"path/to/webview2/include" /link /LIBPATH:"path/to/webview2/lib"
//      WebView2LoaderStatic.lib user32.lib ole32.lib comctl32.lib shlwapi.lib
//
// ============================================================

#include "browser.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow)
{
    // Unused parameters
    (void)hPrevInstance;
    (void)pCmdLine;

    NativeNavBrowser browser;

    if (!browser.Initialize(hInstance, nCmdShow)) {
        MessageBoxW(nullptr,
            L"Error al inicializar NativeNav.\n\n"
            L"Asegúrate de tener Microsoft Edge WebView2 Runtime instalado.\n"
            L"Descárgalo en: https://developer.microsoft.com/en-us/microsoft-edge/webview2/",
            L"NativeNav - Error",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    return browser.Run();
}
