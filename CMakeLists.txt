# ============================================================
# NativeNav - CMake Build Configuration
# ============================================================
# 
# Requisitos:
#   - Visual Studio 2019+ con C++ Desktop Development
#   - Microsoft WebView2 SDK (se descarga automáticamente via NuGet)
#   - CMake 3.20+
#
# Uso:
#   mkdir build && cd build
#   cmake .. -G "Visual Studio 17 2022"
#   cmake --build . --config Release
#
# ============================================================

cmake_minimum_required(VERSION 3.20)
project(NativeNav VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unicode support
add_definitions(-DUNICODE -D_UNICODE)

# ============================================================
# WebView2 SDK via NuGet
# ============================================================

# Check for WebView2 SDK
set(WEBVIEW2_VERSION "1.0.2903.40")
set(NUGET_PACKAGE_DIR "${CMAKE_BINARY_DIR}/packages")
set(WEBVIEW2_DIR "${NUGET_PACKAGE_DIR}/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")

# Download NuGet if not present
if(NOT EXISTS "${CMAKE_BINARY_DIR}/nuget.exe")
    message(STATUS "Descargando NuGet...")
    file(DOWNLOAD
        "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe"
        "${CMAKE_BINARY_DIR}/nuget.exe"
        SHOW_PROGRESS
    )
endif()

# Install WebView2 SDK via NuGet
if(NOT EXISTS "${WEBVIEW2_DIR}")
    message(STATUS "Descargando Microsoft WebView2 SDK ${WEBVIEW2_VERSION}...")
    execute_process(
        COMMAND "${CMAKE_BINARY_DIR}/nuget.exe" install Microsoft.Web.WebView2
            -Version ${WEBVIEW2_VERSION}
            -OutputDirectory "${NUGET_PACKAGE_DIR}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        RESULT_VARIABLE NUGET_RESULT
    )
    if(NOT NUGET_RESULT EQUAL 0)
        message(FATAL_ERROR "Error al descargar WebView2 SDK. Verifica tu conexión a internet.")
    endif()
endif()

# Also get Windows Implementation Libraries (WIL)
set(WIL_VERSION "1.0.240803.1")
set(WIL_DIR "${NUGET_PACKAGE_DIR}/Microsoft.Windows.ImplementationLibrary.${WIL_VERSION}")

if(NOT EXISTS "${WIL_DIR}")
    message(STATUS "Descargando Windows Implementation Libraries...")
    execute_process(
        COMMAND "${CMAKE_BINARY_DIR}/nuget.exe" install Microsoft.Windows.ImplementationLibrary
            -Version ${WIL_VERSION}
            -OutputDirectory "${NUGET_PACKAGE_DIR}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endif()

# ============================================================
# Source files
# ============================================================

set(SOURCES
    src/main.cpp
    src/browser.cpp
)

set(HEADERS
    src/browser.h
)

# ============================================================
# Executable
# ============================================================

add_executable(NativeNav WIN32 ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(NativeNav PRIVATE
    "${WEBVIEW2_DIR}/build/native/include"
    "${WIL_DIR}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

# Determine architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WEBVIEW2_ARCH "x64")
else()
    set(WEBVIEW2_ARCH "x86")
endif()

# Link WebView2 static library
target_link_libraries(NativeNav PRIVATE
    "${WEBVIEW2_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2LoaderStatic.lib"
    user32
    ole32
    oleaut32
    comctl32
    shlwapi
    advapi32
)

# ============================================================
# Post-build: Copy config files and WebView2Loader
# ============================================================

# Copy config directory to output
add_custom_command(TARGET NativeNav POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:NativeNav>/config"
    COMMENT "Copiando archivos de configuración..."
)

# Copy WebView2Loader.dll (for dynamic linking fallback)
if(EXISTS "${WEBVIEW2_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2Loader.dll")
    add_custom_command(TARGET NativeNav POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${WEBVIEW2_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2Loader.dll"
            "$<TARGET_FILE_DIR:NativeNav>"
        COMMENT "Copiando WebView2Loader.dll..."
    )
endif()

# ============================================================
# Install target
# ============================================================

install(TARGETS NativeNav DESTINATION bin)
install(DIRECTORY config/ DESTINATION bin/config)

message(STATUS "")
message(STATUS "=== NativeNav Build Configuration ===")
message(STATUS "  Version:     ${PROJECT_VERSION}")
message(STATUS "  Architecture: ${WEBVIEW2_ARCH}")
message(STATUS "  WebView2 SDK: ${WEBVIEW2_VERSION}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "=====================================")
message(STATUS "")
