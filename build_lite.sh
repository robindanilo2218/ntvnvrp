#!/bin/sh

echo "======================================================="
echo "  NativeNav - Compilacion ultra ligera (MinGW/GCC)"
echo "======================================================="
echo ""

if ! command -v g++ > /dev/null 2>&1; then
    echo "[ERROR] No se encontro g++."
    exit 1
fi

mkdir -p packages

if [ ! -f "nuget.exe" ]; then
    echo "[1/3] Descargando empaquetador NuGet (Solo 7 MB)..."
    curl -s -L -o nuget.exe https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
    if [ $? -ne 0 ]; then
        echo "[ERROR] No se pudo descargar nuget.exe"
        exit 1
    fi
fi

WEBVIEW2_VER="1.0.2903.40"
WEBVIEW2_DIR="packages/Microsoft.Web.WebView2.${WEBVIEW2_VER}"

if [ ! -d "$WEBVIEW2_DIR" ]; then
    echo "[2/3] Descargando recursos de WebView2 SDK (Solo 6 MB)..."
    ./nuget.exe install Microsoft.Web.WebView2 -Version $WEBVIEW2_VER -OutputDirectory packages
    if [ $? -ne 0 ]; then
        echo "[ERROR] Fallo la descarga de WebView2"
        exit 1
    fi
fi

echo "[3/3] Compilando NativeNav con g++..."
g++ -std=c++17 -O3 -mwindows -municode -DUNICODE -D_UNICODE \
    -I"${WEBVIEW2_DIR}/build/native/include" \
    src/main.cpp src/browser.cpp \
    -o NativeNav.exe \
    "${WEBVIEW2_DIR}/build/native/x64/WebView2Loader.dll" \
    -lole32 -loleaut32 -lcomctl32 -lshlwapi -ladvapi32 -luser32 -luuid

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] La compilacion ha fallado. Revisar el error arriba."
    exit 1
fi

echo "Copiando WebView2Loader.dll para ejecutar la aplicacion..."
cp -f "${WEBVIEW2_DIR}/build/native/x64/WebView2Loader.dll" ./

mkdir -p config
[ -f "config/filters.json" ] && cp -f "config/filters.json" ./config/
[ -f "config/search_engines.json" ] && cp -f "config/search_engines.json" ./config/

echo ""
echo "======================================================="
echo "[EXITO] Compilacion completada con exito"
echo "Tu navegador esta listo en: NativeNav.exe"
echo "======================================================="
