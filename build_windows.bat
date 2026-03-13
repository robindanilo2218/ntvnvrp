@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion
title NativeNav - Compilacion

REM Guardar log de todo
set LOGFILE=%~dp0build_log.txt
echo NativeNav Build Log - %date% %time% > "%LOGFILE%"
echo ============================================ >> "%LOGFILE%"

echo.
echo  ======================================================
echo    NativeNav - Sistema de Compilacion v1.2
echo    Navegador Ligero, Privado y Sin Anuncios
echo  ======================================================
echo.
echo  (Si algo falla, revisa el archivo build_log.txt)
echo.

REM Parse arguments
set BUILD_TYPE=Release
if /i "%1"=="debug" set BUILD_TYPE=Debug
if /i "%1"=="clean" goto :clean

REM ============================================================
REM PASO 1: Detectar Visual Studio
REM ============================================================

echo [1/5] Buscando Visual Studio...
echo [1/5] Buscando Visual Studio... >> "%LOGFILE%"

set VCVARSALL=
set VS_FOUND=0

REM Metodo 1: vswhere.exe
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
echo   Buscando vswhere en: %VSWHERE% >> "%LOGFILE%"

if exist "%VSWHERE%" (
    echo   vswhere encontrado >> "%LOGFILE%"
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>^&1`) do (
        set "VS_PATH=%%i"
        echo   VS_PATH=%%i >> "%LOGFILE%"
    )
    if defined VS_PATH (
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARSALL=!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat"
            set VS_FOUND=1
            echo   OK: !VS_PATH! >> "%LOGFILE%"
            echo        Encontrado: !VS_PATH!
        ) else (
            echo   vcvarsall.bat no existe en !VS_PATH! >> "%LOGFILE%"
        )
    ) else (
        echo   VS_PATH no definido - vswhere no encontro VC tools >> "%LOGFILE%"
    )
) else (
    echo   vswhere NO encontrado >> "%LOGFILE%"
)

REM Metodo 2: rutas manuales
if !VS_FOUND! equ 0 (
    echo   Buscando en rutas manuales... >> "%LOGFILE%"
    
    set "CHECK_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "!CHECK_PATH!" (
        set "VCVARSALL=!CHECK_PATH!"
        set VS_FOUND=1
        echo        Encontrado: VS 2022 Community
        echo   Encontrado: !CHECK_PATH! >> "%LOGFILE%"
        goto :vs_found
    )
    set "CHECK_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "!CHECK_PATH!" (
        set "VCVARSALL=!CHECK_PATH!"
        set VS_FOUND=1
        echo        Encontrado: VS 2022 Professional
        echo   Encontrado: !CHECK_PATH! >> "%LOGFILE%"
        goto :vs_found
    )
    set "CHECK_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "!CHECK_PATH!" (
        set "VCVARSALL=!CHECK_PATH!"
        set VS_FOUND=1
        echo        Encontrado: VS 2022 Enterprise
        echo   Encontrado: !CHECK_PATH! >> "%LOGFILE%"
        goto :vs_found
    )
    set "CHECK_PATH=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "!CHECK_PATH!" (
        set "VCVARSALL=!CHECK_PATH!"
        set VS_FOUND=1
        echo        Encontrado: VS 2022 BuildTools
        echo   Encontrado: !CHECK_PATH! >> "%LOGFILE%"
        goto :vs_found
    )
    set "CHECK_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "!CHECK_PATH!" (
        set "VCVARSALL=!CHECK_PATH!"
        set VS_FOUND=1
        echo        Encontrado: VS 2019 Community
        echo   Encontrado: !CHECK_PATH! >> "%LOGFILE%"
        goto :vs_found
    )
    set "CHECK_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "!CHECK_PATH!" (
        set "VCVARSALL=!CHECK_PATH!"
        set VS_FOUND=1
        echo        Encontrado: VS 2019 BuildTools
        echo   Encontrado: !CHECK_PATH! >> "%LOGFILE%"
        goto :vs_found
    )
)

:vs_found
echo   VS_FOUND=!VS_FOUND! >> "%LOGFILE%"

if !VS_FOUND! equ 0 (
    echo.
    echo  ======================================================
    echo  [ERROR] No se encontro Visual Studio
    echo  ======================================================
    echo.
    echo  Para compilar NativeNav necesitas instalar:
    echo.
    echo  Visual Studio Community 2022 (GRATIS):
    echo  https://visualstudio.microsoft.com/downloads/
    echo.
    echo  Durante la instalacion, MARCA:
    echo    [x] "Desktop development with C++"
    echo.
    echo  Despues de instalar, ejecuta este archivo de nuevo.
    echo.
    echo  ERROR: Visual Studio no encontrado >> "%LOGFILE%"
    goto :end
)

REM ============================================================
REM PASO 2: Configurar entorno
REM ============================================================

echo [2/5] Configurando entorno de compilacion...
echo [2/5] Configurando entorno... >> "%LOGFILE%"

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo        Cargando vcvarsall.bat x64...
    echo   Ejecutando: "!VCVARSALL!" x64 >> "%LOGFILE%"
    call "!VCVARSALL!" x64 >> "%LOGFILE%" 2>&1
    if !errorlevel! neq 0 (
        echo  [ERROR] No se pudo configurar el entorno de compilacion.
        echo  ERROR: vcvarsall fallo con codigo !errorlevel! >> "%LOGFILE%"
        goto :end
    )
)

where cl >> "%LOGFILE%" 2>&1
echo        Entorno configurado.

REM ============================================================
REM PASO 3: Verificar CMake
REM ============================================================

echo [3/5] Verificando CMake...
echo [3/5] Verificando CMake... >> "%LOGFILE%"

where cmake >> "%LOGFILE%" 2>&1
if %errorlevel% neq 0 (
    echo.
    echo  [ERROR] CMake no encontrado.
    echo  Deberia venir con Visual Studio.
    echo  O descargalo de: https://cmake.org/download/
    echo  ERROR: CMake no encontrado >> "%LOGFILE%"
    goto :end
)

cmake --version >> "%LOGFILE%" 2>&1
echo        CMake OK.

REM ============================================================
REM PASO 4: Configurar CMake
REM ============================================================

echo [4/5] Configurando proyecto con CMake...
echo [4/5] Configurando CMake... >> "%LOGFILE%"

if not exist "%~dp0build" mkdir "%~dp0build"
cd /d "%~dp0build"

echo   Directorio actual: %CD% >> "%LOGFILE%"

REM Usar NMake ya que tenemos el entorno VS configurado
echo        Usando NMake Makefiles...
echo   Generador: NMake Makefiles >> "%LOGFILE%"

cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% >> "%LOGFILE%" 2>&1

if !errorlevel! neq 0 (
    echo.
    echo  [ERROR] CMake fallo al configurar el proyecto.
    echo  Revisa build_log.txt para mas detalles.
    echo  ERROR: CMake configure fallo >> "%LOGFILE%"
    cd /d "%~dp0"
    goto :end
)

echo        Proyecto configurado.

REM ============================================================
REM PASO 5: Compilar
REM ============================================================

echo [5/5] Compilando NativeNav...
echo        (puede tomar unos segundos...)
echo.
echo [5/5] Compilando... >> "%LOGFILE%"

cmake --build . --config %BUILD_TYPE% >> "%LOGFILE%" 2>&1

if !errorlevel! neq 0 (
    echo.
    echo  [ERROR] La compilacion fallo.
    echo  Revisa build_log.txt para ver los errores.
    echo  ERROR: Build fallo >> "%LOGFILE%"
    cd /d "%~dp0"
    goto :end
)

cd /d "%~dp0"

REM ============================================================
REM EXITO
REM ============================================================

echo  ======================================================
echo.
echo    COMPILACION EXITOSA!
echo.
echo  BUILD OK >> "%LOGFILE%"

set EXE_PATH=
if exist "%~dp0build\%BUILD_TYPE%\NativeNav.exe" (
    set "EXE_PATH=%~dp0build\%BUILD_TYPE%\NativeNav.exe"
) else if exist "%~dp0build\NativeNav.exe" (
    set "EXE_PATH=%~dp0build\NativeNav.exe"
)

if defined EXE_PATH (
    echo    Tu navegador esta en:
    echo    !EXE_PATH!
    echo.
    echo    EXE: !EXE_PATH! >> "%LOGFILE%"
    echo  ======================================================
    echo.
    echo  [1] Abrir NativeNav ahora
    echo  [2] Abrir carpeta del .exe
    echo  [3] Salir
    echo.
    set /p CHOICE="  Opcion (1/2/3): "
    if "!CHOICE!"=="1" start "" "!EXE_PATH!"
    if "!CHOICE!"=="2" (
        for %%F in ("!EXE_PATH!") do explorer "%%~dpF"
    )
) else (
    echo    Ejecutable en carpeta build\
    echo    EXE no encontrado en rutas esperadas >> "%LOGFILE%"
)

goto :end

REM ============================================================
REM LIMPIAR
REM ============================================================

:clean
echo Limpiando...
if exist "%~dp0build" (
    rmdir /s /q "%~dp0build"
    echo Carpeta build eliminada.
)
if exist "%~dp0build_log.txt" del "%~dp0build_log.txt"
echo Limpio.

:end
echo.
echo  Log guardado en: %LOGFILE%
echo.
echo  Presiona cualquier tecla para cerrar...
pause >nul
endlocal
