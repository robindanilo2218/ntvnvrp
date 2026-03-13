@echo off
REM ============================================================
REM NativeNav - Script de Compilación para Windows
REM ============================================================
REM
REM Requisitos:
REM   - Visual Studio 2019+ con "Desktop development with C++"
REM   - CMake 3.20+ (viene incluido con Visual Studio)
REM   - Conexión a internet (para descargar WebView2 SDK)
REM
REM Uso:
REM   build_windows.bat          (compila en Release)
REM   build_windows.bat debug    (compila en Debug)
REM   build_windows.bat clean    (limpia el build)
REM
REM ============================================================

setlocal enabledelayedexpansion

echo.
echo  ╔══════════════════════════════════════════╗
echo  ║     NativeNav - Build System v1.0        ║
echo  ║     Navegador Ligero para Windows        ║
echo  ╚══════════════════════════════════════════╝
echo.

REM Parse arguments
set BUILD_TYPE=Release
if /i "%1"=="debug" set BUILD_TYPE=Debug
if /i "%1"=="clean" goto :clean

REM Check for CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake no encontrado.
    echo.
    echo Opciones para instalar CMake:
    echo   1. Instalar Visual Studio con "Desktop development with C++"
    echo   2. Descargar CMake desde https://cmake.org/download/
    echo.
    goto :error
)

REM Check for Visual Studio
set VS_GENERATOR=
for %%G in (
    "Visual Studio 17 2022"
    "Visual Studio 16 2019"
) do (
    cmake -G %%G --check-system-vars >nul 2>&1
    if !errorlevel! equ 0 (
        set VS_GENERATOR=%%G
        goto :found_vs
    )
)

REM Try Ninja if available
where ninja >nul 2>&1
if %errorlevel% equ 0 (
    set VS_GENERATOR=Ninja
    goto :found_vs
)

echo [ERROR] No se encontró Visual Studio 2019+ ni Ninja.
echo.
echo Instala Visual Studio con "Desktop development with C++":
echo   https://visualstudio.microsoft.com/downloads/
echo.
goto :error

:found_vs
echo [INFO] Generador: %VS_GENERATOR%
echo [INFO] Build Type: %BUILD_TYPE%
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo [1/3] Configurando proyecto con CMake...
echo.

if "%VS_GENERATOR%"=="Ninja" (
    cmake .. -G %VS_GENERATOR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
) else (
    cmake .. -G %VS_GENERATOR% -A x64
)

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Fallo la configuración de CMake.
    goto :error
)

echo.
echo [2/3] Compilando NativeNav...
echo.

cmake --build . --config %BUILD_TYPE% --parallel

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Fallo la compilación.
    goto :error
)

echo.
echo [3/3] Build completado exitosamente!
echo.

REM Find the output
set OUTPUT_DIR=
if exist "%BUILD_TYPE%\NativeNav.exe" set OUTPUT_DIR=%BUILD_TYPE%
if exist "NativeNav.exe" set OUTPUT_DIR=.

if defined OUTPUT_DIR (
    echo  ╔══════════════════════════════════════════╗
    echo  ║  ✅ NativeNav compilado exitosamente!     ║
    echo  ╚══════════════════════════════════════════╝
    echo.
    echo  Ejecutable: build\%OUTPUT_DIR%\NativeNav.exe
    echo.
    echo  Para ejecutar:
    echo    cd build\%OUTPUT_DIR%
    echo    NativeNav.exe
    echo.
    echo  NOTA: Asegúrate de que la carpeta 'config' esté
    echo  junto al ejecutable con los archivos .json
    echo.
) else (
    echo  Ejecutable generado en la carpeta build\
)

cd ..
goto :end

:clean
echo Limpiando build...
if exist build (
    rmdir /s /q build
    echo [OK] Carpeta build eliminada.
) else (
    echo [INFO] No hay nada que limpiar.
)
goto :end

:error
echo.
echo Build fallido. Revisa los errores arriba.
cd ..
exit /b 1

:end
echo.
endlocal
