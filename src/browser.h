#pragma once
// NativeNav - Lightweight Browser for Windows
// browser.h - Browser class declaration

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <commctrl.h>

// WebView2 headers
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "WebView2LoaderStatic.lib")

using namespace Microsoft::WRL;

// ============================================================
// Structures
// ============================================================

struct SearchEngine {
    std::wstring name;
    std::wstring url;
    std::wstring icon;
};

struct ContentFilter {
    bool enabled;
    std::vector<std::string> blocked_selectors;
    std::vector<std::string> blocked_domains;
    std::vector<std::string> cookie_banner_selectors;
    std::vector<std::string> whitelist;
};

// Resource tracking for inspector
enum class ResourceStatus {
    Allowed,
    Blocked,
    Suspicious
};

enum class ResourceType {
    Script,
    Stylesheet,
    Image,
    Font,
    XHR,
    Fetch,
    IFrame,
    Media,
    WebSocket,
    Other
};

struct TrackedResource {
    std::wstring url;
    std::wstring domain;
    ResourceType type;
    ResourceStatus status;
    int64_t size;
    std::wstring mimeType;
    bool isThirdParty;
};

// ============================================================
// Control IDs
// ============================================================
#define IDC_BACK_BTN        1001
#define IDC_FORWARD_BTN     1002
#define IDC_RELOAD_BTN      1003
#define IDC_HOME_BTN        1004
#define IDC_URL_BAR         1005
#define IDC_GO_BTN          1006
#define IDC_SEARCH_COMBO    1007
#define IDC_SHIELD_BTN      1008
#define IDC_SETTINGS_BTN    1009
#define IDC_INSPECTOR_BTN   1010

#define TOOLBAR_HEIGHT      40
#define INSPECTOR_WIDTH     420
#define WM_NAVIGATE         (WM_USER + 1)

// ============================================================
// Browser Class
// ============================================================

class NativeNavBrowser {
public:
    NativeNavBrowser();
    ~NativeNavBrowser();

    // Initialize and run
    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int Run();

    // Window procedure (static + instance)
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Window setup
    bool RegisterWindowClass(HINSTANCE hInstance);
    bool CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
    void CreateToolbar();
    void ResizeControls();

    // WebView2
    void InitializeWebView();
    void NavigateToUrl(const std::wstring& url);
    void NavigateToSearch(const std::wstring& query);
    void GoBack();
    void GoForward();
    void Reload();
    void GoHome();

    // URL processing
    std::wstring ProcessInput(const std::wstring& input);
    bool IsUrl(const std::wstring& input);
    std::wstring GetCurrentSearchUrl();

    // Content filtering
    void LoadFilters();
    void LoadSearchEngines();
    std::string BuildFilterScript();
    std::string BuildCookieBannerScript();
    void InjectContentFilters();
    bool ShouldBlockDomain(const std::wstring& url);

    // Settings
    void OpenSettings();
    std::wstring GetSettingsHtml();
    std::wstring GetHomePageHtml();
    std::wstring GetExePath();
    std::wstring GetConfigPath();

    // Resource Inspector
    void ToggleInspector();
    void InitializeInspectorWebView();
    void TrackResource(const std::wstring& url, ResourceType type, ResourceStatus status, bool isThirdParty);
    void ClearTrackedResources();
    void UpdateInspectorPanel();
    ResourceType ClassifyResourceType(const std::wstring& url, COREWEBVIEW2_WEB_RESOURCE_CONTEXT context);
    ResourceStatus ClassifyResourceStatus(const std::wstring& url);
    std::wstring ExtractDomain(const std::wstring& url);
    bool IsThirdPartyResource(const std::wstring& resourceUrl);
    std::wstring GetInspectorHtml();
    std::wstring GetDownloadWarningHtml(const std::wstring& filename, const std::wstring& url,
                                         const std::wstring& filesize, const std::wstring& mimetype);
    std::string ResourceTypeToString(ResourceType type);
    std::string ResourceStatusToString(ResourceStatus status);

    // Members
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    HWND m_hUrlBar;
    HWND m_hSearchCombo;
    HWND m_hBackBtn;
    HWND m_hForwardBtn;
    HWND m_hReloadBtn;
    HWND m_hHomeBtn;
    HWND m_hGoBtn;
    HWND m_hShieldBtn;
    HWND m_hSettingsBtn;
    HWND m_hInspectorBtn;
    HWND m_hStatusBar;

    ComPtr<ICoreWebView2Environment> m_webViewEnvironment;
    ComPtr<ICoreWebView2Controller> m_webViewController;
    ComPtr<ICoreWebView2> m_webView;

    // Inspector WebView (panel lateral)
    ComPtr<ICoreWebView2Controller> m_inspectorController;
    ComPtr<ICoreWebView2> m_inspectorWebView;

    std::vector<SearchEngine> m_searchEngines;
    ContentFilter m_filters;
    int m_selectedEngine;
    bool m_filtersActive;
    std::wstring m_homeUrl;

    // Inspector state
    bool m_inspectorOpen;
    std::vector<TrackedResource> m_trackedResources;
    std::wstring m_currentPageDomain;
    std::vector<std::string> m_suspiciousDomains;

    static const wchar_t* WINDOW_CLASS;
    static const wchar_t* WINDOW_TITLE;
};
