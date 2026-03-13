# 🌐 NativeNav — Navegador Ligero para Windows

<p align="center">
  <strong>Un navegador web ligero, privado y sin anuncios para Windows</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Plataforma-Windows%2010%2F11-blue" alt="Windows">
  <img src="https://img.shields.io/badge/Lenguaje-C++17-orange" alt="C++17">
  <img src="https://img.shields.io/badge/Motor-WebView2%20(Chromium)-green" alt="WebView2">
  <img src="https://img.shields.io/badge/Licencia-MIT-yellow" alt="MIT">
</p>

---

## ✨ Características

| Característica | Descripción |
|---|---|
| 🚀 **Ligero** | Ejecutable de ~100KB que usa WebView2 (incluido en Windows 10/11) |
| 🌍 **HTML5 Completo** | Soporte total de HTML5, CSS3, JavaScript, WebGL, etc. |
| 🛡️ **Bloqueador de Anuncios** | Bloquea ads a nivel de CSS y de red (sin extensiones) |
| 🍪 **Control de Cookies** | Bloqueo automático de cookies de terceros |
| 🚫 **Anti-Popups** | Elimina banners de cookies, suscripciones y popups molestos |
| 🔍 **Multi-Buscador** | Google, DuckDuckGo, Bing, Brave, Yahoo, Wikipedia, YouTube |
| ⚙️ **Configurable** | Archivos JSON editables para filtros y buscadores |
| 🏠 **Página de Inicio** | Página de inicio moderna con accesos directos |

## 📸 Interfaz

```
┌─────────────────────────────────────────────────────────────────┐
│ [◀][▶][🔄][🏠] [Google ▼] [___URL o búsqueda___] [Ir] [🛡️][⚙️] │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                     🌐 NativeNav                                │
│              Navegador ligero, privado                          │
│                  y sin anuncios                                 │
│                                                                 │
│    🔍Google  ▶️YouTube  📚Wikipedia  💻GitHub  🗨️Reddit  🦆DDG   │
│                                                                 │
│         🛡️ Protección activa  🚫 Ads bloqueados                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 🛠️ Requisitos para Compilar

- **Windows 10/11**
- **Visual Studio 2019 o 2022** con la carga de trabajo "Desktop development with C++"
- **CMake 3.20+** (incluido con Visual Studio)
- **Conexión a internet** (para descargar el SDK de WebView2 automáticamente)

### Requisito en el PC del usuario final:
- **Microsoft Edge WebView2 Runtime** — Ya viene preinstalado en Windows 10 (versión 21H1+) y Windows 11. Si no lo tienes:
  - [Descargar WebView2 Runtime](https://developer.microsoft.com/en-us/microsoft-edge/webview2/)

## 🚀 Compilar

### Opción 1: Script automatizado (recomendado)

```batch
# Abre "Developer Command Prompt for VS 2022" y ejecuta:

cd NativeNav
build_windows.bat

# Para compilar en modo debug:
build_windows.bat debug

# Para limpiar:
build_windows.bat clean
```

### Opción 2: CMake manual

```batch
cd NativeNav
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Opción 3: Compilación directa con MSVC

```batch
# Desde "Developer Command Prompt for VS 2022":

# Primero descarga el WebView2 SDK (NuGet):
nuget install Microsoft.Web.WebView2 -OutputDirectory packages

# Compila:
cl /EHsc /std:c++17 /DUNICODE /D_UNICODE ^
   /I"packages\Microsoft.Web.WebView2.1.0.2903.40\build\native\include" ^
   src\main.cpp src\browser.cpp ^
   /Fe:NativeNav.exe ^
   /link /SUBSYSTEM:WINDOWS ^
   /LIBPATH:"packages\Microsoft.Web.WebView2.1.0.2903.40\build\native\x64" ^
   WebView2LoaderStatic.lib user32.lib ole32.lib oleaut32.lib ^
   comctl32.lib shlwapi.lib advapi32.lib
```

## 📁 Estructura del Proyecto

```
NativeNav/
├── 📄 CMakeLists.txt          # Configuración de build con CMake
├── 📄 build_windows.bat       # Script de compilación automatizado
├── 📄 README.md               # Esta documentación
├── 📄 .gitignore              # Archivos ignorados por Git
│
├── 📁 src/                    # Código fuente
│   ├── 📄 main.cpp            # Punto de entrada de la aplicación
│   ├── 📄 browser.h           # Declaración de la clase NativeNavBrowser
│   └── 📄 browser.cpp         # Implementación del navegador
│
└── 📁 config/                 # Archivos de configuración (editables)
    ├── 📄 filters.json        # Reglas del bloqueador de anuncios y cookies
    └── 📄 search_engines.json # Lista de motores de búsqueda
```

## ⚙️ Configuración

### Motores de Búsqueda (`config/search_engines.json`)

```json
{
  "default": "Google",
  "engines": [
    {
      "name": "Google",
      "url": "https://www.google.com/search?q=",
      "icon": "🔍"
    },
    {
      "name": "DuckDuckGo",
      "url": "https://duckduckgo.com/?q=",
      "icon": "🦆"
    }
  ]
}
```

Para agregar un nuevo buscador, simplemente agrega otro objeto al array `engines`.

### Filtros de Anuncios (`config/filters.json`)

```json
{
  "enabled": true,
  "blocked_selectors": [
    "ins.adsbygoogle",
    "div[class*='ad-container']",
    "div[class*='advertisement']"
  ],
  "blocked_domains": [
    "doubleclick.net",
    "googlesyndication.com"
  ],
  "cookie_banner_selectors": [
    "div[class*='cookie-banner']",
    "div[class*='cookie-consent']"
  ],
  "whitelist": [
    "sitio-que-quieres-permitir.com"
  ]
}
```

#### Secciones del archivo de filtros:

| Sección | Qué hace |
|---------|----------|
| `blocked_selectors` | Selectores CSS de elementos a ocultar (anuncios, banners) |
| `blocked_domains` | Dominios de red a bloquear completamente (servidores de ads) |
| `cookie_banner_selectors` | Selectores CSS de banners de cookies y popups molestos |
| `whitelist` | Sitios donde NO se aplican los filtros |

## 🎯 Cómo Funciona

### Detección Inteligente URL vs Búsqueda

Cuando escribes algo en la barra de direcciones:

| Entrada | Acción |
|---------|--------|
| `google.com` | Navega a `https://google.com` |
| `https://ejemplo.com` | Navega directamente |
| `clima guatemala` | Busca en el motor seleccionado |
| `localhost:3000` | Navega a `http://localhost:3000` |
| `192.168.1.1` | Navega a la IP |
| `settings` | Abre la configuración |
| `home` | Va a la página de inicio |

### Bloqueo de Anuncios (3 capas)

1. **Bloqueo de Red** — Las peticiones a dominios de publicidad se interceptan y bloquean ANTES de descargarse
2. **Ocultamiento CSS** — Los elementos publicitarios se ocultan con CSS inyectado
3. **Eliminación JS** — JavaScript remueve los elementos del DOM para una limpieza completa

### Control de Cookies (3 capas)

1. **Cookies de terceros** — Bloqueadas a nivel de motor de navegación
2. **Banners de cookies** — Eliminados automáticamente con CSS + JS
3. **MutationObserver** — Detecta y elimina banners que aparecen dinámicamente

## 🔧 Uso

### Atajos de la barra de navegación

| Botón | Función |
|-------|---------|
| ◀ | Ir atrás |
| ▶ | Ir adelante |
| ↻ | Recargar página |
| ⌂ | Ir a inicio |
| 🛡️ | Activar/Desactivar bloqueador de anuncios |
| ⚙️ | Abrir configuración |
| `Enter` | Navegar/Buscar |

### 🔍 Inspector de Recursos

Presiona el botón **🔍** en la barra para abrir un panel lateral que muestra:

- **Todos los recursos** que carga cada página (scripts, CSS, imágenes, XHR, etc.)
- **Sistema de semáforo**: 🟢 Permitido, 🟡 Sospechoso, 🔴 Bloqueado
- **Identificación de terceros**: Marca recursos de dominios externos con `3rd`
- **Conteo por tipo**: Scripts, CSS, Imágenes, XHR, Fuentes, Media
- **Detección de archivos peligrosos**: `.exe`, `.msi`, `.bat`, `.scr`, `.vbs`
- **Dominios sospechosos**: Trackers, mineros de cripto, pixels de rastreo

Esto te permite ver exactamente qué carga cada sitio y decidir si es seguro.

### Páginas internas

- `home` o `about:home` — Página de inicio
- `settings` o `nativenav://settings` — Panel de configuración

## 🏗️ Arquitectura Técnica

```
┌─────────────────────────────────────────┐
│            NativeNav.exe (~100KB)        │
│  ┌─────────────────────────────────────┐ │
│  │  Win32 API - Ventana y Controles    │ │
│  │  (Barra de URL, Botones, ComboBox) │ │
│  ├─────────────────────────────────────┤ │
│  │  Content Filter Engine              │ │
│  │  (CSS Injection + JS + Domain Block)│ │
│  ├─────────────────────────────────────┤ │
│  │  WebView2 Controller               │ │
│  │  (Navegación, Eventos, Cookies)     │ │
│  └──────────────┬──────────────────────┘ │
└─────────────────┼───────────────────────┘
                  │
    ┌─────────────▼──────────────┐
    │  Microsoft Edge WebView2    │
    │  Runtime (Chromium-based)   │
    │  ┌───────────────────────┐  │
    │  │ HTML5 / CSS3 / JS     │  │
    │  │ Engine                │  │
    │  └───────────────────────┘  │
    └────────────────────────────┘
```

## 📋 Distribución

Para distribuir NativeNav, solo necesitas:

```
NativeNav/
├── NativeNav.exe          # El ejecutable
├── config/
│   ├── filters.json       # Filtros de anuncios
│   └── search_engines.json # Buscadores
└── (WebView2Loader.dll)   # Solo si no usas linkeo estático
```

El usuario final solo necesita tener **Windows 10/11** con **WebView2 Runtime** (ya viene preinstalado en la mayoría de PCs modernos).

## 📝 Licencia

MIT License — Úsalo, modifícalo y distribúyelo libremente.

---

<p align="center">
  <strong>NativeNav v1.0</strong> — Navegador ligero, privado y sin anuncios 🛡️
</p>
