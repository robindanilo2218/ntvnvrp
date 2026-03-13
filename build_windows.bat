@echo off
REM ============================================================
REM NativeNav - Script de Compilacion para Windows
REM ============================================================
REM
REM Solo dale DOBLE CLIC a este archivo y se compilara
REM automaticamente. No necesitas abrir ningun terminal especial.
REM
REM Requisitos:
REM   - Visual Studio 2019+ con "Desktop development with C++"
REM   - Conexion a internet (primera vez, para descargar WebView2 SDK)
REM
REM ============================================================

setlocal enabledelayedexpansion
title NativeNav - Compilacion

echo.
echo  ======================================================
echo       NativeNav - Sistema de Compilacion v1.1
echo       Navegador Ligero, Privado y Sin Anuncios
echo  ======================================================
echo.

REM Parse arguments
set BUILD_TYPE=Release
if /i "%1"=="debug" set BUILD_TYPE=Debug
if /i "%1"=="clean" goto :clean

REM ============================================================
REM PASO 1: Detectar Visual Studio automaticamente
REM ============================================================

echo [1/5] Buscando Visual Studio...

set VCVARSALL=
set VS_FOUND=0

REM Metodo 1: Usar vswhere.exe (la forma oficial de Microsoft)
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist %VSWHERE% (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set VS_PATH=%%i
    )
    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" (
            set VCVARSALL=!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat
            set VS_FOUND=1
            echo        Encontrado: !VS_PATH!
        )
    )
)

REM Metodo 2: Buscar en rutas comunes
if !VS_FOUND! equ 0 (
    for %%V in (
        "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
        "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    ) do (
        if exist %%~V (
            set VCVARSALL=%%~V
            set VS_FOUND=1
            echo        Encontrado: %%~V
            goto :vs_found
        )
    )
)

:vs_found
if !VS_FOUND! equ 0 (
    echo.
    echo  [ERROR] No se encontro Visual Studio.
    echo.
    echo  Para compilar NativeNav necesitas instalar Visual Studio:
    echo.
    echo    1. Descarga Visual Studio Community (GRATIS):
    echo       https://visualstudio.microsoft.com/downloads/
    echo.
    echo    2. Durante la instalacion, marca:
    echo       [x] "Desktop development with C++"
    echo          (Desarrollo para escritorio con C++)
    echo.
    echo    3. Despues de instalar, dale doble clic a este
    echo       archivo de nuevo.
    echo.
    echo  ======================================================
    echo.
    pause
    exit /b 1
)

REM ============================================================
REM PASO 2: Configurar entorno de compilacion
REM ============================================================

echo [2/5] Configurando entorno de compilacion (x64)...

REM Verificar si ya estamos en un entorno de desarrollador
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo        Ejecutando vcvarsall.bat x64...
    call "!VCVARSALL!" x64 >nul 2>&1
    if %errorlevel% neq 0 (
        echo  [ERROR] No se pudo configurar el entorno.
        pause
        exit /b 1
    )
)
echo        Entorno configurado correctamente.

REM ============================================================
REM PASO 3: Verificar CMake
REM ============================================================

echo [3/5] Verificando CMake...

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo  [ERROR] CMake no encontrado.
    echo  CMake deberia venir incluido con Visual Studio.
    echo  Tambien puedes descargarlo de: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)
echo        CMake encontrado.

REM ============================================================
REM PASO 4: Configurar proyecto con CMake
REM ============================================================

echo [4/5] Configurando proyecto...

if not exist build mkdir build
cd build

REM Detectar generador de CMake disponible
set CMAKE_GEN=
where ninja >nul 2>&1
if %errorlevel% equ 0 (
    set CMAKE_GEN=Ninja
) else (
    REM Intentar Visual Studio generators
    cmake -G "Visual Studio 17 2022" --check-system-vars >nul 2>&1
    if !errorlevel! equ 0 (
        set CMAKE_GEN=Visual Studio 17 2022
    ) else (
        cmake -G "Visual Studio 16 2019" --check-system-vars >nul 2>&1
        if !errorlevel! equ 0 (
            set CMAKE_GEN=Visual Studio 16 2019
        ) else (
            REM Fallback a NMake
            set CMAKE_GEN=NMake Makefiles
        )
    )
)

echo        Generador: !CMAKE_GEN!
echo        Tipo: %BUILD_TYPE%
echo.

if "!CMAKE_GEN!"=="Ninja" (
    cmake .. -G "!CMAKE_GEN!" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% 2>&1
) else if "!CMAKE_GEN!"=="NMake Makefiles" (
    cmake .. -G "!CMAKE_GEN!" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% 2>&1
) else (
    cmake .. -G "!CMAKE_GEN!" -A x64 2>&1
)

if %errorlevel% neq 0 (
    echo.
    echo  [ERROR] Fallo la configuracion de CMake.
    echo  Revisa los errores arriba.
    echo.
    cd ..
    pause
    exit /b 1
)

REM ============================================================
REM PASO 5: Compilar
REM ============================================================

echo.
echo [5/5] Compilando NativeNav...
echo        (esto puede tomar unos segundos)
echo.

cmake --build . --config %BUILD_TYPE% --parallel 2>&1

if %errorlevel% neq 0 (
    echo.
    echo  [ERROR] Fallo la compilacion.
    echo  Revisa los errores arriba.
    echo.
    cd ..
    pause
    exit /b 1
)

cd ..

REM ============================================================
REM RESULTADO
REM ============================================================

echo.
echo  ======================================================
echo.
echo    COMPILACION EXITOSA!
echo.

REM Buscar el ejecutable
set EXE_PATH=
if exist "build\%BUILD_TYPE%\NativeNav.exe" (
    set EXE_PATH=build\%BUILD_TYPE%\NativeNav.exe
) else if exist "build\NativeNav.exe" (
    set EXE_PATH=build\NativeNav.exe
)

if defined EXE_PATH (
    echo    Tu navegador esta listo en:
    echo    %CD%\!EXE_PATH!
    echo.
    echo  ======================================================
    echo.
    echo  Que deseas hacer?
    echo.
    echo    [1] Abrir NativeNav ahora
    echo    [2] Abrir la carpeta donde esta el .exe
    echo    [3] Salir
    echo.
    set /p CHOICE="  Elige una opcion (1/2/3): "
    
    if "!CHOICE!"=="1" (
        echo.
        echo  Abriendo NativeNav...
        start "" "!EXE_PATH!"
    ) else if "!CHOICE!"=="2" (
        for %%F in ("!EXE_PATH!") do set EXE_DIR=%%~dpF
        explorer "!EXE_DIR!"
    ) else (
        echo.
        echo  Puedes ejecutar tu navegador en cualquier momento:
        echo  %CD%\!EXE_PATH!
    )
) else (
    echo    Ejecutable generado en la carpeta build\
)

echo.
echo  Gracias por usar NativeNav!
echo.
pause
goto :eof

REM ============================================================
REM LIMPIAR BUILD
REM ============================================================

:clean
echo Limpiando build...
if exist build (
    rmdir /s /q build
    echo [OK] Carpeta build eliminada.
) else (
    echo [INFO] No hay nada que limpiar.
)
echo.
pause

endlocal
