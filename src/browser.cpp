// NativeNav - Lightweight Browser for Windows
// browser.cpp - Browser class implementation

#include "browser.h"
#include <shellapi.h>
#include <shlwapi.h>
#include <locale>
#include <codecvt>

#pragma comment(lib, "shlwapi.lib")

const wchar_t* NativeNavBrowser::WINDOW_CLASS = L"NativeNavBrowserClass";
const wchar_t* NativeNavBrowser::WINDOW_TITLE = L"NativeNav - Navegador Ligero";

// ============================================================
// Helper: Simple JSON string value extraction
// ============================================================
static std::string JsonGetString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    pos = json.find("\"", pos + searchKey.length() + 1);
    if (pos == std::string::npos) return "";
    size_t end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

static bool JsonGetBool(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return false;
    pos = json.find(":", pos);
    if (pos == std::string::npos) return false;
    std::string rest = json.substr(pos + 1, 10);
    return rest.find("true") != std::string::npos;
}

static std::vector<std::string> JsonGetStringArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return result;
    size_t arrStart = json.find("[", pos);
    if (arrStart == std::string::npos) return result;
    size_t arrEnd = json.find("]", arrStart);
    if (arrEnd == std::string::npos) return result;
    std::string arr = json.substr(arrStart + 1, arrEnd - arrStart - 1);
    
    size_t p = 0;
    while ((p = arr.find("\"", p)) != std::string::npos) {
        size_t e = arr.find("\"", p + 1);
        if (e == std::string::npos) break;
        result.push_back(arr.substr(p + 1, e - p - 1));
        p = e + 1;
    }
    return result;
}

// Extract array of objects from JSON - returns each object as a string
static std::vector<std::string> JsonGetObjectArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return result;
    size_t arrStart = json.find("[", pos);
    if (arrStart == std::string::npos) return result;
    
    int depth = 0;
    size_t objStart = 0;
    for (size_t i = arrStart + 1; i < json.size(); i++) {
        if (json[i] == '{') {
            if (depth == 0) objStart = i;
            depth++;
        } else if (json[i] == '}') {
            depth--;
            if (depth == 0) {
                result.push_back(json.substr(objStart, i - objStart + 1));
            }
        } else if (json[i] == ']' && depth == 0) {
            break;
        }
    }
    return result;
}

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}

static std::string WideToUtf8(const std::wstring& str) {
    if (str.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

static std::string ReadFileToString(const std::wstring& path) {
    std::ifstream file(WideToUtf8(path));
    if (!file.is_open()) {
        // Try wide path
        std::ifstream file2(path.c_str());
        if (!file2.is_open()) return "";
        return std::string((std::istreambuf_iterator<char>(file2)),
                           std::istreambuf_iterator<char>());
    }
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

static std::string UrlEncode(const std::wstring& input) {
    std::string utf8 = WideToUtf8(input);
    std::string encoded;
    for (char c : utf8) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else if (c == ' ') {
            encoded += '+';
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            encoded += buf;
        }
    }
    return encoded;
}

// ============================================================
// Constructor / Destructor
// ============================================================

NativeNavBrowser::NativeNavBrowser()
    : m_hInstance(nullptr)
    , m_hWnd(nullptr)
    , m_hUrlBar(nullptr)
    , m_hSearchCombo(nullptr)
    , m_hBackBtn(nullptr)
    , m_hForwardBtn(nullptr)
    , m_hReloadBtn(nullptr)
    , m_hHomeBtn(nullptr)
    , m_hGoBtn(nullptr)
    , m_hShieldBtn(nullptr)
    , m_hSettingsBtn(nullptr)
    , m_hInspectorBtn(nullptr)
    , m_hStatusBar(nullptr)
    , m_selectedEngine(0)
    , m_filtersActive(true)
    , m_homeUrl(L"about:home")
    , m_inspectorOpen(false)
{
    // Suspicious domains for resource classification
    m_suspiciousDomains = {
        "cdn.track", "pixel.", "beacon.", "telemetry.", "metrics.",
        "collect.", "log.", "stat.", "counter.", "tracker.",
        "clicktrack", "impression", "popunder", "malware",
        "crypto-mining", "coinhive", "coin-hive", "cryptoloot",
        "minero.", "miner.", "webminer"
    };
}

NativeNavBrowser::~NativeNavBrowser() {
}

// ============================================================
// Initialize
// ============================================================

bool NativeNavBrowser::Initialize(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // Load configuration
    LoadSearchEngines();
    LoadFilters();

    // Create window
    if (!RegisterWindowClass(hInstance)) return false;
    if (!CreateMainWindow(hInstance, nCmdShow)) return false;

    // Initialize WebView2
    InitializeWebView();

    return true;
}

int NativeNavBrowser::Run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        // Handle Enter key in URL bar
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND focus = GetFocus();
            if (focus == m_hUrlBar) {
                // Trigger the Go button
                SendMessage(m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_GO_BTN, BN_CLICKED), (LPARAM)m_hGoBtn);
                // Prevent the edit control from beeping by not translating this message
                continue;
            }
        }
        
        // Let standard dialog messages process (for tab navigation, etc)
        if (!IsDialogMessage(m_hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    CoUninitialize();
    return (int)msg.wParam;
}

// ============================================================
// Window Registration & Creation
// ============================================================

bool NativeNavBrowser::RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StaticWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    return RegisterClassExW(&wc) != 0;
}

bool NativeNavBrowser::CreateMainWindow(HINSTANCE hInstance, int nCmdShow) {
    m_hWnd = CreateWindowExW(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr, nullptr,
        hInstance,
        this  // Pass this pointer for WM_CREATE
    );

    if (!m_hWnd) return false;

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
    return true;
}

// ============================================================
// Toolbar Creation
// ============================================================

void NativeNavBrowser::CreateToolbar() {
    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    HFONT hBtnFont = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");

    int x = 4;
    int y = 4;
    int btnW = 36;
    int btnH = 32;

    // Back button
    m_hBackBtn = CreateWindowExW(0, L"BUTTON", L"\x25C0",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, btnW, btnH, m_hWnd, (HMENU)IDC_BACK_BTN, m_hInstance, nullptr);
    SendMessage(m_hBackBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    x += btnW + 2;

    // Forward button
    m_hForwardBtn = CreateWindowExW(0, L"BUTTON", L"\x25B6",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, btnW, btnH, m_hWnd, (HMENU)IDC_FORWARD_BTN, m_hInstance, nullptr);
    SendMessage(m_hForwardBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    x += btnW + 2;

    // Reload button
    m_hReloadBtn = CreateWindowExW(0, L"BUTTON", L"\x21BB",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, btnW, btnH, m_hWnd, (HMENU)IDC_RELOAD_BTN, m_hInstance, nullptr);
    SendMessage(m_hReloadBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    x += btnW + 2;

    // Home button
    m_hHomeBtn = CreateWindowExW(0, L"BUTTON", L"\x2302",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, btnW, btnH, m_hWnd, (HMENU)IDC_HOME_BTN, m_hInstance, nullptr);
    SendMessage(m_hHomeBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
    x += btnW + 6;

    // Search engine combo box
    m_hSearchCombo = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        x, y, 140, 200, m_hWnd, (HMENU)IDC_SEARCH_COMBO, m_hInstance, nullptr);
    SendMessage(m_hSearchCombo, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Populate search engines
    for (size_t i = 0; i < m_searchEngines.size(); i++) {
        SendMessageW(m_hSearchCombo, CB_ADDSTRING, 0,
                     (LPARAM)m_searchEngines[i].name.c_str());
    }
    SendMessage(m_hSearchCombo, CB_SETCURSEL, m_selectedEngine, 0);
    x += 144;

    // URL bar - will be resized in ResizeControls
    m_hUrlBar = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, 400, btnH, m_hWnd, (HMENU)IDC_URL_BAR, m_hInstance, nullptr);
    SendMessage(m_hUrlBar, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(m_hUrlBar, EM_SETCUEBANNER, TRUE, (LPARAM)L"Ingresa una URL o busca algo...");

    // Go button (will be positioned in ResizeControls)
    m_hGoBtn = CreateWindowExW(0, L"BUTTON", L"Ir",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, y, 40, btnH, m_hWnd, (HMENU)IDC_GO_BTN, m_hInstance, nullptr);
    SendMessage(m_hGoBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Shield button (ad blocker toggle)
    m_hShieldBtn = CreateWindowExW(0, L"BUTTON",
        m_filtersActive ? L"\U0001F6E1" : L"\u26A0",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, y, btnW, btnH, m_hWnd, (HMENU)IDC_SHIELD_BTN, m_hInstance, nullptr);
    SendMessage(m_hShieldBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);

    // Inspector button
    m_hInspectorBtn = CreateWindowExW(0, L"BUTTON", L"\U0001F50D",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, y, btnW, btnH, m_hWnd, (HMENU)IDC_INSPECTOR_BTN, m_hInstance, nullptr);
    SendMessage(m_hInspectorBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);

    // Settings button
    m_hSettingsBtn = CreateWindowExW(0, L"BUTTON", L"\x2699",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, y, btnW, btnH, m_hWnd, (HMENU)IDC_SETTINGS_BTN, m_hInstance, nullptr);
    SendMessage(m_hSettingsBtn, WM_SETFONT, (WPARAM)hBtnFont, TRUE);

    ResizeControls();
}

void NativeNavBrowser::ResizeControls() {
    if (!m_hWnd) return;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    int btnW = 36;
    int y = 4;
    int btnH = 32;

    // Calculate URL bar position and width
    // Right side: go(40) + shield(36) + inspector(36) + settings(36) + spacing
    int urlBarX = 4 + (btnW + 2) * 4 + 6 + 144;
    int rightButtonsWidth = 40 + 4 + (btnW + 2) * 3 + 8;
    int urlBarWidth = width - urlBarX - rightButtonsWidth;
    if (urlBarWidth < 100) urlBarWidth = 100;

    MoveWindow(m_hUrlBar, urlBarX, y, urlBarWidth, btnH, TRUE);

    int goX = urlBarX + urlBarWidth + 4;
    MoveWindow(m_hGoBtn, goX, y, 40, btnH, TRUE);

    int shieldX = goX + 44;
    MoveWindow(m_hShieldBtn, shieldX, y, btnW, btnH, TRUE);

    int inspectorX = shieldX + btnW + 2;
    MoveWindow(m_hInspectorBtn, inspectorX, y, btnW, btnH, TRUE);

    int settingsX = inspectorX + btnW + 2;
    MoveWindow(m_hSettingsBtn, settingsX, y, btnW, btnH, TRUE);

    // Resize WebView - account for inspector panel
    if (m_webViewController) {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.top = TOOLBAR_HEIGHT;
        if (m_inspectorOpen) {
            bounds.right -= INSPECTOR_WIDTH;
        }
        m_webViewController->put_Bounds(bounds);
    }

    // Resize Inspector WebView panel
    if (m_inspectorController) {
        if (m_inspectorOpen) {
            RECT inspBounds;
            inspBounds.left = width - INSPECTOR_WIDTH;
            inspBounds.top = TOOLBAR_HEIGHT;
            inspBounds.right = width;
            inspBounds.bottom = height;
            m_inspectorController->put_Bounds(inspBounds);
            m_inspectorController->put_IsVisible(TRUE);
        } else {
            m_inspectorController->put_IsVisible(FALSE);
        }
    }
}

// ============================================================
// Window Procedure
// ============================================================

LRESULT CALLBACK NativeNavBrowser::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NativeNavBrowser* pThis = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<NativeNavBrowser*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<NativeNavBrowser*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->WndProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT NativeNavBrowser::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        m_hWnd = hwnd;
        CreateToolbar();
        return 0;

    case WM_SIZE:
        ResizeControls();
        return 0;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BACK_BTN:
            GoBack();
            break;
        case IDC_FORWARD_BTN:
            GoForward();
            break;
        case IDC_RELOAD_BTN:
            Reload();
            break;
        case IDC_HOME_BTN:
            GoHome();
            break;
        case IDC_GO_BTN:
        {
            wchar_t buffer[4096] = {};
            GetWindowTextW(m_hUrlBar, buffer, 4096);
            std::wstring input(buffer);
            if (!input.empty()) {
                std::wstring url = ProcessInput(input);
                NavigateToUrl(url);
            }
            break;
        }
        case IDC_SEARCH_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                m_selectedEngine = (int)SendMessage(m_hSearchCombo, CB_GETCURSEL, 0, 0);
            }
            break;
        case IDC_SHIELD_BTN:
            m_filtersActive = !m_filtersActive;
            SetWindowTextW(m_hShieldBtn, m_filtersActive ? L"\U0001F6E1" : L"\u26A0");
            // Re-inject or remove filters
            if (m_webView) {
                if (m_filtersActive) {
                    InjectContentFilters();
                } else {
                    // Remove filters by reloading
                    Reload();
                }
            }
            break;
        case IDC_INSPECTOR_BTN:
            ToggleInspector();
            break;
        case IDC_SETTINGS_BTN:
            OpenSettings();
            break;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// ============================================================
// WebView2 Initialization
// ============================================================

class CustomEnvironmentOptions : public ICoreWebView2EnvironmentOptions {
private:
    ULONG refCount = 1;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override {
        if (!ppv) return E_POINTER;
        if (IsEqualIID(riid, IID_ICoreWebView2EnvironmentOptions) || IsEqualIID(riid, IID_IUnknown)) {
            *ppv = static_cast<ICoreWebView2EnvironmentOptions*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refCount; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = --refCount;
        if (count == 0) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE get_AdditionalBrowserArguments(LPWSTR *value) override {
        if (!value) return E_POINTER;
        const wchar_t* args = L"--block-third-party-cookies --disable-features=ThirdPartyCookies";
        size_t len = wcslen(args) + 1;
        *value = (LPWSTR)CoTaskMemAlloc(len * sizeof(wchar_t));
        if (*value) wcscpy(*value, args);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE put_AdditionalBrowserArguments(LPCWSTR value) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE get_Language(LPWSTR *value) override { *value = nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE put_Language(LPCWSTR value) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE get_TargetCompatibleBrowserVersion(LPWSTR *value) override { *value = nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE put_TargetCompatibleBrowserVersion(LPCWSTR value) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE get_AllowSingleSignOnUsingOSPrimaryAccount(BOOL *value) override { if(value) *value = FALSE; return S_OK; }
    HRESULT STDMETHODCALLTYPE put_AllowSingleSignOnUsingOSPrimaryAccount(BOOL value) override { return S_OK; }
};

void NativeNavBrowser::InitializeWebView() {
    // Create WebView2 environment with custom options
    ComPtr<ICoreWebView2EnvironmentOptions> options = new CustomEnvironmentOptions();

    std::wstring userDataFolder = GetExePath() + L"\\NativeNav_UserData";

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, userDataFolder.c_str(), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(result) || !env) {
                    wchar_t msg[256];
                    swprintf(msg, 256, L"Failed to create WebView2 Environment.\nHRESULT: 0x%08X", result);
                    MessageBoxW(m_hWnd, msg, L"WebView2 Error", MB_ICONERROR | MB_OK);
                    return result;
                }

                m_webViewEnvironment = env;

                env->CreateCoreWebView2Controller(m_hWnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (FAILED(result) || !controller) {
                                wchar_t msg[256];
                                swprintf(msg, 256, L"Failed to create WebView2 Controller.\nHRESULT: 0x%08X", result);
                                MessageBoxW(m_hWnd, msg, L"WebView2 Error", MB_ICONERROR | MB_OK);
                                return result;
                            }

                            m_webViewController = controller;
                            controller->get_CoreWebView2(&m_webView);

                            // Set bounds
                            RECT bounds;
                            GetClientRect(m_hWnd, &bounds);
                            bounds.top = TOOLBAR_HEIGHT;
                            m_webViewController->put_Bounds(bounds);

                            // Configure settings
                            ComPtr<ICoreWebView2Settings> settings;
                            m_webView->get_Settings(&settings);
                            settings->put_IsScriptEnabled(TRUE);
                            settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                            settings->put_IsWebMessageEnabled(TRUE);
                            settings->put_AreDevToolsEnabled(FALSE);
                            settings->put_IsStatusBarEnabled(FALSE);

                            // Block third-party cookies via settings if available
                            ComPtr<ICoreWebView2Settings> settings2;
                            if (SUCCEEDED(settings->QueryInterface(IID_ICoreWebView2Settings, (void**)&settings2))) {
                                // Additional settings if available
                            }

                            // Event: Navigation completed - inject filters
                            m_webView->add_NavigationCompleted(
                                Callback<ICoreWebView2NavigationCompletedEventHandler>(
                                    [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                                        if (m_filtersActive) {
                                            InjectContentFilters();
                                        }
                                        return S_OK;
                                    }).Get(), nullptr);

                            // Event: Source changed - update URL bar
                            m_webView->add_SourceChanged(
                                Callback<ICoreWebView2SourceChangedEventHandler>(
                                    [this](ICoreWebView2* sender, ICoreWebView2SourceChangedEventArgs* args) -> HRESULT {
                                        LPWSTR uri;
                                        sender->get_Source(&uri);
                                        SetWindowTextW(m_hUrlBar, uri);
                                        CoTaskMemFree(uri);
                                        return S_OK;
                                    }).Get(), nullptr);

                            // Event: Title changed - update window title
                            m_webView->add_DocumentTitleChanged(
                                Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                                    [this](ICoreWebView2* sender, IUnknown* args) -> HRESULT {
                                        LPWSTR title;
                                        sender->get_DocumentTitle(&title);
                                        std::wstring windowTitle = std::wstring(title) + L" - NativeNav";
                                        SetWindowTextW(m_hWnd, windowTitle.c_str());
                                        CoTaskMemFree(title);
                                        return S_OK;
                                    }).Get(), nullptr);

                            // Event: Navigation starting - clear tracked resources
                            m_webView->add_NavigationStarting(
                                Callback<ICoreWebView2NavigationStartingEventHandler>(
                                    [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                        LPWSTR uri;
                                        args->get_Uri(&uri);
                                        m_currentPageDomain = ExtractDomain(std::wstring(uri));
                                        CoTaskMemFree(uri);
                                        ClearTrackedResources();
                                        return S_OK;
                                    }).Get(), nullptr);

                            // Event: Block requests to ad domains + track resources for inspector
                            m_webView->AddWebResourceRequestedFilter(L"*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
                            m_webView->add_WebResourceRequested(
                                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                                    [this](ICoreWebView2* sender, ICoreWebView2WebResourceRequestedEventArgs* args) -> HRESULT {
                                        ComPtr<ICoreWebView2WebResourceRequest> request;
                                        args->get_Request(&request);
                                        LPWSTR uri;
                                        request->get_Uri(&uri);
                                        std::wstring url(uri);
                                        CoTaskMemFree(uri);

                                        // Get resource context for classification
                                        COREWEBVIEW2_WEB_RESOURCE_CONTEXT context;
                                        args->get_ResourceContext(&context);

                                        ResourceType rType = ClassifyResourceType(url, context);
                                        bool isThirdParty = IsThirdPartyResource(url);
                                        ResourceStatus rStatus = ResourceStatus::Allowed;

                                        bool blocked = false;
                                        if (m_filtersActive && ShouldBlockDomain(url)) {
                                            ComPtr<ICoreWebView2WebResourceResponse> response;
                                            m_webViewEnvironment->CreateWebResourceResponse(
                                                nullptr, 403, L"Blocked", L"", &response);
                                            args->put_Response(response.Get());
                                            rStatus = ResourceStatus::Blocked;
                                            blocked = true;
                                        }

                                        if (!blocked) {
                                            rStatus = ClassifyResourceStatus(url);
                                        }

                                        // Track the resource for the inspector
                                        TrackResource(url, rType, rStatus, isThirdParty);

                                        return S_OK;
                                    }).Get(), nullptr);

                            // Navigate to home page
                            GoHome();

                            return S_OK;
                        }).Get());

                return S_OK;
            }).Get());

    if (FAILED(hr)) {
        wchar_t msg[256];
        swprintf(msg, 256, L"Immediate failure in CreateCoreWebView2EnvironmentWithOptions.\nHRESULT: 0x%08X\nIs WebView2Loader.dll present?", hr);
        MessageBoxW(m_hWnd, msg, L"WebView2 Error", MB_ICONERROR | MB_OK);
    }
}

// ============================================================
// Navigation
// ============================================================

void NativeNavBrowser::NavigateToUrl(const std::wstring& url) {
    if (!m_webView) {
        MessageBoxW(m_hWnd, L"m_webView is null! WebView2 failed to initialize in time.", L"Navigation Error", MB_ICONERROR | MB_OK);
        return;
    }

    if (url == L"about:home" || url == L"nativenav://home") {
        std::wstring html = GetHomePageHtml();
        m_webView->NavigateToString(html.c_str());
        SetWindowTextW(m_hUrlBar, L"");
        return;
    }

    if (url == L"nativenav://settings") {
        std::wstring html = GetSettingsHtml();
        m_webView->NavigateToString(html.c_str());
        SetWindowTextW(m_hUrlBar, L"nativenav://settings");
        return;
    }

    m_webView->Navigate(url.c_str());
    SetWindowTextW(m_hUrlBar, url.c_str());
}

void NativeNavBrowser::NavigateToSearch(const std::wstring& query) {
    std::wstring searchUrl = GetCurrentSearchUrl();
    std::string encoded = UrlEncode(query);
    std::wstring fullUrl = searchUrl + Utf8ToWide(encoded);
    NavigateToUrl(fullUrl);
}

void NativeNavBrowser::GoBack() {
    if (m_webView) {
        BOOL canGoBack;
        m_webView->get_CanGoBack(&canGoBack);
        if (canGoBack) m_webView->GoBack();
    }
}

void NativeNavBrowser::GoForward() {
    if (m_webView) {
        BOOL canGoForward;
        m_webView->get_CanGoForward(&canGoForward);
        if (canGoForward) m_webView->GoForward();
    }
}

void NativeNavBrowser::Reload() {
    if (m_webView) m_webView->Reload();
}

void NativeNavBrowser::GoHome() {
    NavigateToUrl(L"about:home");
}

// ============================================================
// URL Processing
// ============================================================

std::wstring NativeNavBrowser::ProcessInput(const std::wstring& input) {
    std::wstring trimmed = input;
    // Trim whitespace
    size_t start = trimmed.find_first_not_of(L" \t\r\n");
    size_t end = trimmed.find_last_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return L"about:home";
    trimmed = trimmed.substr(start, end - start + 1);

    // Check for internal URLs
    if (trimmed == L"about:home" || trimmed == L"home") return L"about:home";
    if (trimmed == L"nativenav://settings" || trimmed == L"settings") return L"nativenav://settings";

    // Check if it's a URL
    if (IsUrl(trimmed)) {
        // Add https:// if no protocol specified
        if (trimmed.find(L"://") == std::wstring::npos) {
            trimmed = L"https://" + trimmed;
        }
        return trimmed;
    }

    // Otherwise, treat as search query
    std::wstring searchUrl = GetCurrentSearchUrl();
    std::string encoded = UrlEncode(trimmed);
    return searchUrl + Utf8ToWide(encoded);
}

bool NativeNavBrowser::IsUrl(const std::wstring& input) {
    // Has protocol?
    if (input.find(L"http://") == 0 || input.find(L"https://") == 0 ||
        input.find(L"file://") == 0 || input.find(L"ftp://") == 0) {
        return true;
    }

    // Has common TLD?
    std::vector<std::wstring> tlds = {
        L".com", L".org", L".net", L".edu", L".gov", L".io", L".co",
        L".dev", L".app", L".me", L".info", L".biz", L".tv", L".cc",
        L".gt", L".mx", L".es", L".ar", L".br", L".cl", L".pe",
        L".uk", L".de", L".fr", L".it", L".ru", L".cn", L".jp",
        L".xyz", L".tech", L".online", L".site", L".store"
    };

    for (const auto& tld : tlds) {
        if (input.find(tld) != std::wstring::npos) return true;
    }

    // Contains dots and no spaces (likely a domain)
    if (input.find(L'.') != std::wstring::npos && input.find(L' ') == std::wstring::npos) {
        return true;
    }

    // localhost or IP address
    if (input.find(L"localhost") == 0 || input.find(L"127.0.0.1") == 0 ||
        input.find(L"192.168.") == 0 || input.find(L"10.") == 0) {
        return true;
    }

    return false;
}

std::wstring NativeNavBrowser::GetCurrentSearchUrl() {
    if (m_selectedEngine >= 0 && m_selectedEngine < (int)m_searchEngines.size()) {
        return m_searchEngines[m_selectedEngine].url;
    }
    return L"https://www.google.com/search?q=";
}

// ============================================================
// Content Filtering
// ============================================================

void NativeNavBrowser::LoadFilters() {
    std::wstring configPath = GetConfigPath() + L"\\filters.json";
    std::string json = ReadFileToString(configPath);
    
    if (json.empty()) {
        // Default filters if file not found
        m_filters.enabled = true;
        m_filters.blocked_selectors = {
            "ins.adsbygoogle", "div[class*='ad-']", "div[class*='advertisement']",
            "iframe[src*='ads']", "div[id*='google_ads']"
        };
        m_filters.blocked_domains = {
            "doubleclick.net", "googlesyndication.com", "googleadservices.com",
            "adservice.google.com", "googletagmanager.com"
        };
        m_filters.cookie_banner_selectors = {
            "div[class*='cookie-banner']", "div[class*='cookie-consent']",
            "div[class*='gdpr']", "div[class*='cc-window']"
        };
        return;
    }

    m_filters.enabled = JsonGetBool(json, "enabled");
    m_filters.blocked_selectors = JsonGetStringArray(json, "blocked_selectors");
    m_filters.blocked_domains = JsonGetStringArray(json, "blocked_domains");
    m_filters.cookie_banner_selectors = JsonGetStringArray(json, "cookie_banner_selectors");
    m_filters.whitelist = JsonGetStringArray(json, "whitelist");
    m_filtersActive = m_filters.enabled;
}

void NativeNavBrowser::LoadSearchEngines() {
    std::wstring configPath = GetConfigPath() + L"\\search_engines.json";
    std::string json = ReadFileToString(configPath);

    if (json.empty()) {
        // Default search engines
        m_searchEngines.push_back({L"Google", L"https://www.google.com/search?q=", L"G"});
        m_searchEngines.push_back({L"DuckDuckGo", L"https://duckduckgo.com/?q=", L"D"});
        m_searchEngines.push_back({L"Bing", L"https://www.bing.com/search?q=", L"B"});
        return;
    }

    std::string defaultEngine = JsonGetString(json, "default");
    auto engines = JsonGetObjectArray(json, "engines");

    for (size_t i = 0; i < engines.size(); i++) {
        SearchEngine se;
        se.name = Utf8ToWide(JsonGetString(engines[i], "name"));
        se.url = Utf8ToWide(JsonGetString(engines[i], "url"));
        se.icon = Utf8ToWide(JsonGetString(engines[i], "icon"));
        m_searchEngines.push_back(se);

        if (JsonGetString(engines[i], "name") == defaultEngine) {
            m_selectedEngine = (int)i;
        }
    }

    if (m_searchEngines.empty()) {
        m_searchEngines.push_back({L"Google", L"https://www.google.com/search?q=", L"G"});
    }
}

std::string NativeNavBrowser::BuildFilterScript() {
    std::string script = "(function() {\n";
    script += "  var style = document.createElement('style');\n";
    script += "  style.id = 'nativenav-adblock';\n";
    script += "  style.textContent = '";

    for (const auto& sel : m_filters.blocked_selectors) {
        script += sel + ",";
    }
    // Remove trailing comma and add rule
    if (!m_filters.blocked_selectors.empty()) {
        script.pop_back(); // remove last comma
    }
    script += " { display: none !important; visibility: hidden !important; height: 0 !important; "
              "width: 0 !important; overflow: hidden !important; position: absolute !important; "
              "pointer-events: none !important; }';\n";

    script += "  var existing = document.getElementById('nativenav-adblock');\n";
    script += "  if (existing) existing.remove();\n";
    script += "  document.head.appendChild(style);\n";

    // Also remove elements via JS for more aggressive blocking
    script += "  var selectors = [";
    for (size_t i = 0; i < m_filters.blocked_selectors.size(); i++) {
        script += "'" + m_filters.blocked_selectors[i] + "'";
        if (i < m_filters.blocked_selectors.size() - 1) script += ",";
    }
    script += "];\n";

    script += "  selectors.forEach(function(sel) {\n";
    script += "    try {\n";
    script += "      document.querySelectorAll(sel).forEach(function(el) { el.remove(); });\n";
    script += "    } catch(e) {}\n";
    script += "  });\n";

    script += "})();\n";
    return script;
}

std::string NativeNavBrowser::BuildCookieBannerScript() {
    std::string script = "(function() {\n";
    script += "  var cookieStyle = document.createElement('style');\n";
    script += "  cookieStyle.id = 'nativenav-cookies';\n";
    script += "  cookieStyle.textContent = '";

    for (const auto& sel : m_filters.cookie_banner_selectors) {
        script += sel + ",";
    }
    if (!m_filters.cookie_banner_selectors.empty()) {
        script.pop_back();
    }
    script += " { display: none !important; visibility: hidden !important; }';\n";

    script += "  var existing = document.getElementById('nativenav-cookies');\n";
    script += "  if (existing) existing.remove();\n";
    script += "  document.head.appendChild(cookieStyle);\n";

    // Remove cookie banner elements
    script += "  var cookieSelectors = [";
    for (size_t i = 0; i < m_filters.cookie_banner_selectors.size(); i++) {
        script += "'" + m_filters.cookie_banner_selectors[i] + "'";
        if (i < m_filters.cookie_banner_selectors.size() - 1) script += ",";
    }
    script += "];\n";

    script += "  function removeCookieBanners() {\n";
    script += "    cookieSelectors.forEach(function(sel) {\n";
    script += "      try {\n";
    script += "        document.querySelectorAll(sel).forEach(function(el) { el.remove(); });\n";
    script += "      } catch(e) {}\n";
    script += "    });\n";
    // Also remove overlay/backdrop that blocks scrolling
    script += "    document.body.style.overflow = 'auto';\n";
    script += "    document.documentElement.style.overflow = 'auto';\n";
    script += "    var overlays = document.querySelectorAll('[class*=\"overlay\"]');\n";
    script += "    overlays.forEach(function(el) {\n";
    script += "      if (el.querySelector('[class*=\"cookie\"]') || el.querySelector('[class*=\"consent\"]')) {\n";
    script += "        el.remove();\n";
    script += "      }\n";
    script += "    });\n";
    script += "  }\n";

    // Run immediately and with delays (some banners load late)
    script += "  removeCookieBanners();\n";
    script += "  setTimeout(removeCookieBanners, 1000);\n";
    script += "  setTimeout(removeCookieBanners, 3000);\n";
    script += "  setTimeout(removeCookieBanners, 5000);\n";

    // MutationObserver to catch dynamically added banners
    script += "  var observer = new MutationObserver(function(mutations) {\n";
    script += "    removeCookieBanners();\n";
    script += "  });\n";
    script += "  observer.observe(document.body, { childList: true, subtree: true });\n";
    script += "  setTimeout(function() { observer.disconnect(); }, 10000);\n";

    script += "})();\n";
    return script;
}

void NativeNavBrowser::InjectContentFilters() {
    if (!m_webView || !m_filtersActive) return;

    // Check whitelist
    LPWSTR currentUri;
    m_webView->get_Source(&currentUri);
    std::wstring currentUrl(currentUri);
    CoTaskMemFree(currentUri);

    std::string currentUrlUtf8 = WideToUtf8(currentUrl);
    for (const auto& whitelisted : m_filters.whitelist) {
        if (currentUrlUtf8.find(whitelisted) != std::string::npos) {
            return; // Don't filter whitelisted sites
        }
    }

    // Inject ad blocker CSS + JS
    std::string adScript = BuildFilterScript();
    m_webView->ExecuteScript(Utf8ToWide(adScript).c_str(), nullptr);

    // Inject cookie banner remover
    std::string cookieScript = BuildCookieBannerScript();
    m_webView->ExecuteScript(Utf8ToWide(cookieScript).c_str(), nullptr);
}

bool NativeNavBrowser::ShouldBlockDomain(const std::wstring& url) {
    std::string urlUtf8 = WideToUtf8(url);
    // Convert to lowercase for comparison
    std::transform(urlUtf8.begin(), urlUtf8.end(), urlUtf8.begin(), ::tolower);

    for (const auto& domain : m_filters.blocked_domains) {
        std::string domainLower = domain;
        std::transform(domainLower.begin(), domainLower.end(), domainLower.begin(), ::tolower);
        if (urlUtf8.find(domainLower) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ============================================================
// Settings
// ============================================================

void NativeNavBrowser::OpenSettings() {
    NavigateToUrl(L"nativenav://settings");
}

std::wstring NativeNavBrowser::GetExePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring fullPath(path);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return fullPath.substr(0, lastSlash);
    }
    return fullPath;
}

std::wstring NativeNavBrowser::GetConfigPath() {
    return GetExePath() + L"\\config";
}

std::wstring NativeNavBrowser::GetHomePageHtml() {
    return LR"(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>NativeNav - Inicio</title>
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #0f0f23 0%, #1a1a3e 50%, #0d1b2a 100%);
        color: #e0e0e0;
        min-height: 100vh;
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
    }
    .logo {
        font-size: 72px;
        margin-bottom: 10px;
        text-shadow: 0 0 30px rgba(100, 200, 255, 0.5);
    }
    h1 {
        font-size: 48px;
        font-weight: 300;
        margin-bottom: 8px;
        background: linear-gradient(90deg, #64b5f6, #42a5f5, #90caf9);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
    }
    .subtitle {
        color: #90a4ae;
        font-size: 16px;
        margin-bottom: 40px;
    }
    .search-box {
        display: flex;
        align-items: center;
        background: rgba(255,255,255,0.08);
        border: 1px solid rgba(255,255,255,0.15);
        border-radius: 30px;
        padding: 8px 20px;
        width: 580px;
        max-width: 90vw;
        transition: all 0.3s;
    }
    .search-box:focus-within {
        border-color: #42a5f5;
        box-shadow: 0 0 20px rgba(66, 165, 245, 0.3);
        background: rgba(255,255,255,0.12);
    }
    .search-box input {
        flex: 1;
        background: none;
        border: none;
        outline: none;
        color: #fff;
        font-size: 18px;
        padding: 8px;
    }
    .search-box input::placeholder { color: #78909c; }
    .search-icon { font-size: 24px; margin-right: 8px; opacity: 0.6; }
    .shortcuts {
        display: flex;
        gap: 20px;
        margin-top: 40px;
        flex-wrap: wrap;
        justify-content: center;
    }
    .shortcut {
        display: flex;
        flex-direction: column;
        align-items: center;
        text-decoration: none;
        color: #b0bec5;
        padding: 16px;
        border-radius: 12px;
        transition: all 0.3s;
        width: 100px;
    }
    .shortcut:hover {
        background: rgba(255,255,255,0.08);
        transform: translateY(-3px);
        color: #fff;
    }
    .shortcut-icon {
        font-size: 32px;
        margin-bottom: 8px;
        width: 56px;
        height: 56px;
        display: flex;
        align-items: center;
        justify-content: center;
        border-radius: 14px;
        background: rgba(255,255,255,0.06);
    }
    .shortcut span { font-size: 12px; }
    .stats {
        position: fixed;
        bottom: 20px;
        display: flex;
        gap: 30px;
        color: #546e7a;
        font-size: 13px;
    }
    .stats span { display: flex; align-items: center; gap: 6px; }
    .shield { color: #4caf50; }
</style>
</head>
<body>
    <div class="logo">🌐</div>
    <h1>NativeNav</h1>
    <p class="subtitle">Navegador ligero, privado y sin anuncios</p>

    <div class="shortcuts">
        <a class="shortcut" href="https://www.google.com" title="Google">
            <div class="shortcut-icon">🔍</div>
            <span>Google</span>
        </a>
        <a class="shortcut" href="https://www.youtube.com" title="YouTube">
            <div class="shortcut-icon">▶️</div>
            <span>YouTube</span>
        </a>
        <a class="shortcut" href="https://www.wikipedia.org" title="Wikipedia">
            <div class="shortcut-icon">📚</div>
            <span>Wikipedia</span>
        </a>
        <a class="shortcut" href="https://github.com" title="GitHub">
            <div class="shortcut-icon">💻</div>
            <span>GitHub</span>
        </a>
        <a class="shortcut" href="https://www.reddit.com" title="Reddit">
            <div class="shortcut-icon">🗨️</div>
            <span>Reddit</span>
        </a>
        <a class="shortcut" href="https://duckduckgo.com" title="DuckDuckGo">
            <div class="shortcut-icon">🦆</div>
            <span>DuckDuckGo</span>
        </a>
    </div>

    <div class="stats">
        <span class="shield">🛡️ Protección activa</span>
        <span>🚫 Anuncios bloqueados</span>
        <span>🍪 Cookies de terceros bloqueadas</span>
    </div>
</body>
</html>)";
}

std::wstring NativeNavBrowser::GetSettingsHtml() {
    // Build the selectors lists for display
    std::string adSelectors, cookieSelectors, blockedDomains, whitelistSites;

    for (const auto& s : m_filters.blocked_selectors) {
        adSelectors += s + "\\n";
    }
    for (const auto& s : m_filters.cookie_banner_selectors) {
        cookieSelectors += s + "\\n";
    }
    for (const auto& s : m_filters.blocked_domains) {
        blockedDomains += s + "\\n";
    }
    for (const auto& s : m_filters.whitelist) {
        whitelistSites += s + "\\n";
    }

    std::wstring html = LR"(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<title>NativeNav - Configuración</title>
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: #121212;
        color: #e0e0e0;
        padding: 30px;
        max-width: 900px;
        margin: 0 auto;
    }
    h1 {
        font-size: 32px;
        margin-bottom: 8px;
        color: #64b5f6;
    }
    .subtitle { color: #78909c; margin-bottom: 30px; }
    .section {
        background: #1e1e1e;
        border: 1px solid #333;
        border-radius: 12px;
        padding: 24px;
        margin-bottom: 20px;
    }
    .section h2 {
        font-size: 20px;
        margin-bottom: 4px;
        display: flex;
        align-items: center;
        gap: 10px;
    }
    .section h2 .icon { font-size: 24px; }
    .section p { color: #90a4ae; font-size: 14px; margin-bottom: 16px; }
    textarea {
        width: 100%;
        height: 150px;
        background: #2a2a2a;
        border: 1px solid #444;
        border-radius: 8px;
        color: #e0e0e0;
        font-family: 'Cascadia Code', 'Consolas', monospace;
        font-size: 13px;
        padding: 12px;
        resize: vertical;
    }
    textarea:focus { outline: none; border-color: #42a5f5; }
    label {
        display: flex;
        align-items: center;
        gap: 10px;
        margin-bottom: 12px;
        cursor: pointer;
    }
    .toggle {
        position: relative;
        width: 50px;
        height: 26px;
    }
    .toggle input { opacity: 0; width: 0; height: 0; }
    .slider {
        position: absolute;
        top: 0; left: 0; right: 0; bottom: 0;
        background: #555;
        border-radius: 13px;
        transition: 0.3s;
        cursor: pointer;
    }
    .slider:before {
        content: '';
        position: absolute;
        width: 20px;
        height: 20px;
        left: 3px;
        bottom: 3px;
        background: white;
        border-radius: 50%;
        transition: 0.3s;
    }
    .toggle input:checked + .slider { background: #4caf50; }
    .toggle input:checked + .slider:before { transform: translateX(24px); }
    .info-box {
        background: rgba(66, 165, 245, 0.1);
        border-left: 3px solid #42a5f5;
        padding: 12px 16px;
        border-radius: 0 8px 8px 0;
        margin-top: 16px;
        font-size: 13px;
        color: #90caf9;
    }
    .badge {
        display: inline-block;
        padding: 2px 8px;
        border-radius: 4px;
        font-size: 12px;
        font-weight: bold;
    }
    .badge-green { background: rgba(76, 175, 80, 0.2); color: #4caf50; }
    .badge-red { background: rgba(244, 67, 54, 0.2); color: #f44336; }
    .badge-blue { background: rgba(66, 165, 245, 0.2); color: #42a5f5; }
    footer {
        text-align: center;
        color: #546e7a;
        margin-top: 40px;
        font-size: 13px;
    }
</style>
</head>
<body>
    <h1>⚙️ Configuración de NativeNav</h1>
    <p class="subtitle">Personaliza tu experiencia de navegación</p>

    <div class="section">
        <h2><span class="icon">🛡️</span> Bloqueador de Anuncios</h2>
        <p>Oculta elementos publicitarios de las páginas web</p>
        <label>
            <div class="toggle">
                <input type="checkbox" id="adBlockEnabled" checked>
                <span class="slider"></span>
            </div>
            <span>Activar bloqueador de anuncios</span>
            <span class="badge badge-green">ACTIVO</span>
        </label>
        <h3 style="margin: 12px 0 8px; font-size: 14px; color: #b0bec5;">Selectores CSS a bloquear (uno por línea):</h3>
        <textarea id="adSelectors" placeholder="div[class*='ad-']&#10;ins.adsbygoogle&#10;...">)" +
    Utf8ToWide(adSelectors) + LR"(</textarea>
        <div class="info-box">
            💡 Cada línea es un selector CSS. Los elementos que coincidan serán ocultados automáticamente.
        </div>
    </div>

    <div class="section">
        <h2><span class="icon">🚫</span> Dominios Bloqueados</h2>
        <p>Las peticiones a estos dominios serán bloqueadas completamente</p>
        <textarea id="blockedDomains" placeholder="doubleclick.net&#10;googlesyndication.com&#10;...">)" +
    Utf8ToWide(blockedDomains) + LR"(</textarea>
        <div class="info-box">
            🔒 Las peticiones de red a estos dominios se bloquean antes de que se descarguen, ahorrando ancho de banda.
        </div>
    </div>

    <div class="section">
        <h2><span class="icon">🍪</span> Control de Cookies y Popups</h2>
        <p>Elimina banners de cookies, popups de suscripción y otros elementos molestos</p>
        <label>
            <div class="toggle">
                <input type="checkbox" id="cookieBlockEnabled" checked>
                <span class="slider"></span>
            </div>
            <span>Bloquear cookies de terceros</span>
            <span class="badge badge-green">ACTIVO</span>
        </label>
        <label>
            <div class="toggle">
                <input type="checkbox" id="cookieBannerEnabled" checked>
                <span class="slider"></span>
            </div>
            <span>Auto-ocultar banners de cookies</span>
        </label>
        <h3 style="margin: 12px 0 8px; font-size: 14px; color: #b0bec5;">Selectores de banners de cookies/popups:</h3>
        <textarea id="cookieSelectors" placeholder="div[class*='cookie-banner']&#10;div[class*='gdpr']&#10;...">)" +
    Utf8ToWide(cookieSelectors) + LR"(</textarea>
    </div>

    <div class="section">
        <h2><span class="icon">✅</span> Lista Blanca (Whitelist)</h2>
        <p>Sitios donde NO se aplicarán los filtros</p>
        <textarea id="whitelist" placeholder="ejemplo.com&#10;sitio-confiable.org&#10;..." style="height: 100px;">)" +
    Utf8ToWide(whitelistSites) + LR"(</textarea>
        <div class="info-box">
            ⚡ Si un sitio no funciona correctamente con los filtros activos, agrégalo aquí.
        </div>
    </div>

    <div class="section">
        <h2><span class="icon">🔍</span> Motores de Búsqueda</h2>
        <p>El motor de búsqueda se puede cambiar desde la barra de navegación</p>
        <table style="width: 100%; border-collapse: collapse; margin-top: 12px;">
            <tr style="text-align: left; border-bottom: 1px solid #333;">
                <th style="padding: 8px; color: #78909c;">Motor</th>
                <th style="padding: 8px; color: #78909c;">URL de búsqueda</th>
            </tr>
            <tr style="border-bottom: 1px solid #222;">
                <td style="padding: 8px;">🔍 Google</td>
                <td style="padding: 8px; font-family: monospace; font-size: 12px; color: #78909c;">https://www.google.com/search?q=</td>
            </tr>
            <tr style="border-bottom: 1px solid #222;">
                <td style="padding: 8px;">🦆 DuckDuckGo</td>
                <td style="padding: 8px; font-family: monospace; font-size: 12px; color: #78909c;">https://duckduckgo.com/?q=</td>
            </tr>
            <tr style="border-bottom: 1px solid #222;">
                <td style="padding: 8px;">🔎 Bing</td>
                <td style="padding: 8px; font-family: monospace; font-size: 12px; color: #78909c;">https://www.bing.com/search?q=</td>
            </tr>
            <tr style="border-bottom: 1px solid #222;">
                <td style="padding: 8px;">🦁 Brave</td>
                <td style="padding: 8px; font-family: monospace; font-size: 12px; color: #78909c;">https://search.brave.com/search?q=</td>
            </tr>
            <tr style="border-bottom: 1px solid #222;">
                <td style="padding: 8px;">📚 Wikipedia</td>
                <td style="padding: 8px; font-family: monospace; font-size: 12px; color: #78909c;">https://es.wikipedia.org/wiki/Special:Search?search=</td>
            </tr>
            <tr>
                <td style="padding: 8px;">▶️ YouTube</td>
                <td style="padding: 8px; font-family: monospace; font-size: 12px; color: #78909c;">https://www.youtube.com/results?search_query=</td>
            </tr>
        </table>
        <div class="info-box">
            📝 Para agregar más motores, edita el archivo <code>config/search_engines.json</code>
        </div>
    </div>

    <footer>
        <p>NativeNav v1.0 — Navegador ligero, privado y sin anuncios</p>
        <p style="margin-top: 4px;">Los archivos de configuración se encuentran en la carpeta <code>config/</code></p>
    </footer>
</body>
</html>)";

    return html;
}

// ============================================================
// Resource Inspector
// ============================================================

std::wstring NativeNavBrowser::ExtractDomain(const std::wstring& url) {
    size_t start = url.find(L"://");
    if (start == std::wstring::npos) return url;
    start += 3;
    size_t end = url.find(L'/', start);
    if (end == std::wstring::npos) end = url.length();
    std::wstring host = url.substr(start, end - start);
    // Remove port
    size_t portPos = host.find(L':');
    if (portPos != std::wstring::npos) host = host.substr(0, portPos);
    return host;
}

bool NativeNavBrowser::IsThirdPartyResource(const std::wstring& resourceUrl) {
    std::wstring resDomain = ExtractDomain(resourceUrl);
    if (m_currentPageDomain.empty() || resDomain.empty()) return false;
    // Compare root domains (last 2 parts)
    auto getRootDomain = [](const std::wstring& d) -> std::wstring {
        size_t lastDot = d.rfind(L'.');
        if (lastDot == std::wstring::npos) return d;
        size_t prevDot = d.rfind(L'.', lastDot - 1);
        if (prevDot == std::wstring::npos) return d;
        return d.substr(prevDot + 1);
    };
    return getRootDomain(resDomain) != getRootDomain(m_currentPageDomain);
}

ResourceType NativeNavBrowser::ClassifyResourceType(const std::wstring& url, COREWEBVIEW2_WEB_RESOURCE_CONTEXT context) {
    switch (context) {
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_SCRIPT: return ResourceType::Script;
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_STYLESHEET: return ResourceType::Stylesheet;
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_IMAGE: return ResourceType::Image;
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FONT: return ResourceType::Font;
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_XML_HTTP_REQUEST: return ResourceType::XHR;
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_FETCH: return ResourceType::Fetch;
        case COREWEBVIEW2_WEB_RESOURCE_CONTEXT_MEDIA: return ResourceType::Media;
        default: break;
    }
    // Fallback: check URL extension
    std::wstring lower = url;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.find(L".js") != std::wstring::npos) return ResourceType::Script;
    if (lower.find(L".css") != std::wstring::npos) return ResourceType::Stylesheet;
    if (lower.find(L".png") != std::wstring::npos || lower.find(L".jpg") != std::wstring::npos ||
        lower.find(L".gif") != std::wstring::npos || lower.find(L".svg") != std::wstring::npos ||
        lower.find(L".webp") != std::wstring::npos || lower.find(L".ico") != std::wstring::npos)
        return ResourceType::Image;
    if (lower.find(L".woff") != std::wstring::npos || lower.find(L".ttf") != std::wstring::npos)
        return ResourceType::Font;
    return ResourceType::Other;
}

ResourceStatus NativeNavBrowser::ClassifyResourceStatus(const std::wstring& url) {
    std::string urlUtf8 = WideToUtf8(url);
    std::transform(urlUtf8.begin(), urlUtf8.end(), urlUtf8.begin(), ::tolower);
    for (const auto& susp : m_suspiciousDomains) {
        if (urlUtf8.find(susp) != std::string::npos) return ResourceStatus::Suspicious;
    }
    // Check for suspicious file extensions
    if (urlUtf8.find(".exe") != std::string::npos || urlUtf8.find(".msi") != std::string::npos ||
        urlUtf8.find(".bat") != std::string::npos || urlUtf8.find(".cmd") != std::string::npos ||
        urlUtf8.find(".scr") != std::string::npos || urlUtf8.find(".vbs") != std::string::npos)
        return ResourceStatus::Suspicious;
    return ResourceStatus::Allowed;
}

std::string NativeNavBrowser::ResourceTypeToString(ResourceType type) {
    switch (type) {
        case ResourceType::Script: return "Script";
        case ResourceType::Stylesheet: return "CSS";
        case ResourceType::Image: return "Imagen";
        case ResourceType::Font: return "Fuente";
        case ResourceType::XHR: return "XHR";
        case ResourceType::Fetch: return "Fetch";
        case ResourceType::IFrame: return "iFrame";
        case ResourceType::Media: return "Media";
        case ResourceType::WebSocket: return "WebSocket";
        default: return "Otro";
    }
}

std::string NativeNavBrowser::ResourceStatusToString(ResourceStatus status) {
    switch (status) {
        case ResourceStatus::Allowed: return "allowed";
        case ResourceStatus::Blocked: return "blocked";
        case ResourceStatus::Suspicious: return "suspicious";
        default: return "unknown";
    }
}

void NativeNavBrowser::TrackResource(const std::wstring& url, ResourceType type, ResourceStatus status, bool isThirdParty) {
    TrackedResource r;
    r.url = url;
    r.domain = ExtractDomain(url);
    r.type = type;
    r.status = status;
    r.size = 0;
    r.isThirdParty = isThirdParty;
    m_trackedResources.push_back(r);

    // Update inspector panel if open
    if (m_inspectorOpen && m_inspectorWebView) {
        UpdateInspectorPanel();
    }
}

void NativeNavBrowser::ClearTrackedResources() {
    m_trackedResources.clear();
}

void NativeNavBrowser::ToggleInspector() {
    m_inspectorOpen = !m_inspectorOpen;

    if (m_inspectorOpen && !m_inspectorController) {
        InitializeInspectorWebView();
    } else {
        ResizeControls();
        if (m_inspectorOpen && m_inspectorWebView) {
            UpdateInspectorPanel();
        }
    }
}

void NativeNavBrowser::InitializeInspectorWebView() {
    if (!m_webViewEnvironment) return;

    m_webViewEnvironment->CreateCoreWebView2Controller(m_hWnd,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                if (FAILED(result) || !controller) return result;
                m_inspectorController = controller;
                controller->get_CoreWebView2(&m_inspectorWebView);

                ComPtr<ICoreWebView2Settings> settings;
                m_inspectorWebView->get_Settings(&settings);
                settings->put_AreDevToolsEnabled(FALSE);
                settings->put_IsStatusBarEnabled(FALSE);
                settings->put_AreDefaultContextMenusEnabled(FALSE);

                ResizeControls();
                UpdateInspectorPanel();
                return S_OK;
            }).Get());
}

void NativeNavBrowser::UpdateInspectorPanel() {
    if (!m_inspectorWebView) return;
    std::wstring html = GetInspectorHtml();
    m_inspectorWebView->NavigateToString(html.c_str());
}

std::wstring NativeNavBrowser::GetInspectorHtml() {
    // Count resources by type and status
    int scripts = 0, css = 0, images = 0, xhr = 0, fonts = 0, media = 0, other = 0;
    int blocked = 0, suspicious = 0, thirdParty = 0;

    for (const auto& r : m_trackedResources) {
        switch (r.type) {
            case ResourceType::Script: scripts++; break;
            case ResourceType::Stylesheet: css++; break;
            case ResourceType::Image: images++; break;
            case ResourceType::XHR: case ResourceType::Fetch: xhr++; break;
            case ResourceType::Font: fonts++; break;
            case ResourceType::Media: media++; break;
            default: other++; break;
        }
        if (r.status == ResourceStatus::Blocked) blocked++;
        if (r.status == ResourceStatus::Suspicious) suspicious++;
        if (r.isThirdParty) thirdParty++;
    }

    int total = (int)m_trackedResources.size();

    // Build resource rows HTML
    std::string rows;
    for (const auto& r : m_trackedResources) {
        std::string statusClass = ResourceStatusToString(r.status);
        std::string typeStr = ResourceTypeToString(r.type);
        std::string icon;
        switch (r.type) {
            case ResourceType::Script: icon = "📄"; break;
            case ResourceType::Stylesheet: icon = "🎨"; break;
            case ResourceType::Image: icon = "🖼"; break;
            case ResourceType::Font: icon = "🔤"; break;
            case ResourceType::XHR: case ResourceType::Fetch: icon = "📡"; break;
            case ResourceType::Media: icon = "🎬"; break;
            default: icon = "📦"; break;
        }
        std::string statusIcon;
        if (r.status == ResourceStatus::Blocked) statusIcon = "🔴";
        else if (r.status == ResourceStatus::Suspicious) statusIcon = "🟡";
        else statusIcon = "🟢";

        std::string domain = WideToUtf8(r.domain);
        std::string shortUrl = WideToUtf8(r.url);
        if (shortUrl.length() > 60) shortUrl = shortUrl.substr(0, 57) + "...";

        rows += "<div class='res " + statusClass + "'>";
        rows += "<span class='status-icon'>" + statusIcon + "</span>";
        rows += "<span class='type-icon'>" + icon + "</span>";
        rows += "<div class='res-info'>";
        rows += "<div class='res-domain'>" + domain;
        if (r.isThirdParty) rows += " <span class='tp'>3rd</span>";
        rows += "</div>";
        rows += "<div class='res-url' title='" + WideToUtf8(r.url) + "'>" + shortUrl + "</div>";
        rows += "</div>";
        rows += "<span class='res-type'>" + typeStr + "</span>";
        rows += "</div>";
    }

    std::string html = R"(<!DOCTYPE html><html><head><meta charset='UTF-8'>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',sans-serif;background:#1a1a2e;color:#e0e0e0;font-size:12px;overflow-x:hidden}
.header{background:#16213e;padding:12px;border-bottom:1px solid #333;position:sticky;top:0;z-index:10}
.header h2{font-size:14px;color:#64b5f6;margin-bottom:8px}
.stats{display:flex;gap:6px;flex-wrap:wrap}
.stat{padding:3px 8px;border-radius:4px;font-size:11px;font-weight:bold}
.stat-total{background:rgba(100,181,246,.15);color:#64b5f6}
.stat-blocked{background:rgba(244,67,54,.15);color:#f44336}
.stat-suspicious{background:rgba(255,193,7,.15);color:#ffc107}
.stat-3p{background:rgba(156,39,176,.15);color:#ce93d8}
.section{padding:8px 12px}
.section h3{font-size:12px;color:#78909c;margin:8px 0 4px;display:flex;align-items:center;gap:6px}
.res{display:flex;align-items:center;gap:6px;padding:5px 8px;border-bottom:1px solid #222;transition:.2s}
.res:hover{background:rgba(255,255,255,.04)}
.res.blocked{opacity:.5;text-decoration:line-through}
.res.suspicious{border-left:2px solid #ffc107}
.status-icon{font-size:10px;flex-shrink:0}
.type-icon{font-size:13px;flex-shrink:0}
.res-info{flex:1;min-width:0}
.res-domain{font-size:11px;color:#90a4ae;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.res-url{font-size:10px;color:#546e7a;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;font-family:monospace}
.res-type{font-size:10px;color:#78909c;background:#222;padding:2px 6px;border-radius:3px;flex-shrink:0}
.tp{background:rgba(156,39,176,.2);color:#ce93d8;padding:1px 4px;border-radius:3px;font-size:9px}
.empty{text-align:center;padding:40px 20px;color:#546e7a}
.legend{padding:8px 12px;border-top:1px solid #333;font-size:10px;color:#546e7a;display:flex;gap:12px}
.legend span{display:flex;align-items:center;gap:3px}
</style></head><body>
<div class='header'>
<h2>🔍 Inspector de Recursos</h2>
<div class='stats'>
<span class='stat stat-total'>)" + std::to_string(total) + R"( total</span>
<span class='stat stat-blocked'>🔴 )" + std::to_string(blocked) + R"( bloqueados</span>
<span class='stat stat-suspicious'>🟡 )" + std::to_string(suspicious) + R"( sospechosos</span>
<span class='stat stat-3p'>🔗 )" + std::to_string(thirdParty) + R"( terceros</span>
</div>
</div>
<div class='section'>
<h3>📄 Scripts ()" + std::to_string(scripts) + R"() · 🎨 CSS ()" + std::to_string(css) +
R"() · 🖼 Img ()" + std::to_string(images) + R"() · 📡 XHR ()" + std::to_string(xhr) + R"()</h3>
</div>
<div class='section'>)";

    if (rows.empty()) {
        html += "<div class='empty'>🔍 Navega a un sitio para ver los recursos cargados</div>";
    } else {
        html += rows;
    }

    html += R"(</div>
<div class='legend'>
<span>🟢 Permitido</span>
<span>🟡 Sospechoso</span>
<span>🔴 Bloqueado</span>
<span class='tp'>3rd = Tercero</span>
</div>
</body></html>)";

    return Utf8ToWide(html);
}

std::wstring NativeNavBrowser::GetDownloadWarningHtml(const std::wstring& filename, const std::wstring& url,
                                                       const std::wstring& filesize, const std::wstring& mimetype) {
    // This HTML is shown when a potentially dangerous download is detected
    return L"<html><body style='font-family:Segoe UI;background:#1a1a2e;color:#e0e0e0;padding:30px;text-align:center'>"
           L"<h2 style='color:#ffc107'>⚠️ Descarga Detectada</h2>"
           L"<p>Archivo: <b>" + filename + L"</b></p>"
           L"<p>Tipo: " + mimetype + L"</p>"
           L"<p>Origen: <code style='color:#78909c'>" + url + L"</code></p>"
           L"<p style='color:#f44336;margin-top:20px'>⚠️ Este tipo de archivo puede contener software malicioso.</p>"
           L"</body></html>";
}

