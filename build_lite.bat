@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   NativeNav - Compilacion ultra ligera (MinGW/GCC)
echo =======================================================
echo.

REM Verificar si g++ esta instalado
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] No se encontro g++.
    echo.
    echo Para compilar sin Visual Studio, haras esto ^(Solo gastara 89 MB^):
    echo 1. Entra a: https://github.com/skeeto/w64devkit/releases
    echo 2. Descarga el archivo "w64devkit-1.XX.X.zip" ^(son 89 MB^).
    echo 3. Extraelo en una carpeta en tu PC.
    echo 4. Ve a la carpeta extraida y dale doble clic a "w64devkit.exe".
    echo 5. Se abrira una ventana negra. Ahi dentro escribe:
    echo    cd "C:\Users\robin\Documents\Antigravity Projects\NativeNav"
    echo    build_lite.bat
    echo.
    exit /b 1
)

if not exist "packages" mkdir packages

if not exist "nuget.exe" (
    echo [1/3] Descargando empaquetador NuGet ^(Solo 7 MB^)...
    curl -s -L -o nuget.exe https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
    if !errorlevel! neq 0 (
        echo [ERROR] No se pudo descargar nuget.exe
        exit /b 1
    )
)

set WEBVIEW2_VER=1.0.2903.40
set WEBVIEW2_DIR=packages\Microsoft.Web.WebView2.%WEBVIEW2_VER%

if not exist "%WEBVIEW2_DIR%" (
    echo [2/3] Descargando recursos de WebView2 SDK ^(Solo 6 MB^)...
    .\nuget.exe install Microsoft.Web.WebView2 -Version %WEBVIEW2_VER% -OutputDirectory packages
    if !errorlevel! neq 0 (
        echo [ERROR] Fallo la descarga de WebView2
        exit /b 1
    )
)

echo [3/3] Compilando NativeNav con g++...
g++ -std=c++17 -O3 -mwindows -DUNICODE -D_UNICODE ^
    -I"%WEBVIEW2_DIR%\build\native\include" ^
    src\main.cpp src\browser.cpp ^
    -o NativeNav.exe ^
    "%WEBVIEW2_DIR%\build\native\x64\WebView2Loader.dll" ^
    -lole32 -loleaut32 -lcomctl32 -lshlwapi -ladvapi32 -luser32

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] La compilacion ha fallado. Revisar el error arriba.
    exit /b 1
)

echo Copiando WebView2Loader.dll para ejecutar la aplicacion...
copy /y "%WEBVIEW2_DIR%\build\native\x64\WebView2Loader.dll" .\ >nul

if not exist "config" mkdir config
if exist "config\filters.json" copy /y "config\filters.json" .\config\ >nul
if exist "config\search_engines.json" copy /y "config\search_engines.json" .\config\ >nul

echo.
echo =======================================================
echo [EXITO] Compilacion completada con exito
echo Tu navegador esta listo en: NativeNav.exe
echo =======================================================
