#include <windows.h>
#include <wrl.h>
#include <wil/com.h>
#include <webview2.h>
#include <shlwapi.h>
#include <string>
#include <ctime>
#include <random>
#include <fstream>

#include "resource.h"

#pragma comment(lib, "Shlwapi.lib")

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)
#pragma warning(disable: 4995)

int timex = 10;
int rd = 1;
int js = 153;
int index = 0;
int override = 0;

using namespace Microsoft::WRL;

wil::com_ptr<ICoreWebView2Controller> webViewController;
wil::com_ptr<ICoreWebView2> webView;

wchar_t exePath[MAX_PATH];

INT_PTR CALLBACK TextEditorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void SaveToFile(HWND hDlg);

std::wstring GetExecutableDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    PathRemoveFileSpec(path);
    return std::wstring(path);
}

void GenerateRandomFilename(char* filename, size_t size) {
    static std::mt19937 gen(static_cast<unsigned>(std::time(nullptr)));
    static int currentIndex = 0;

    int randomNumber;
    if (override != 0) {
        randomNumber = override;
    }
    else if (rd == 0) {
        randomNumber = currentIndex;
        currentIndex = (currentIndex + 1) % js;
    }
    else {
        randomNumber = gen() % js;
    }

    std::snprintf(filename, size, "%d.js", randomNumber);

    std::ofstream log("debug.log", std::ios::app);
    log << "Generated filename: " << filename << "\n";
    log.close();
}

void load() {
    SetCurrentDirectoryW(exePath);
    FILE* fp = fopen("pj5s.ini", "r");
    if (fp && fscanf(fp, "%d %d %d %d", &rd, &js, &timex, &override) == 4) {
        fclose(fp);
    }
    else {
        rd = 1;
        js = 153;
        timex = 30;
        override = 0;
        if (fp) fclose(fp);
        fp = fopen("pj5s.ini", "w");
        if (fp) {
            fprintf(fp, "%d %d %d %d", rd, js, timex, override);
            fclose(fp);
        }
        MessageBox(NULL, L"pj5s.ini was empty, missing, or invalid. Created with defaults: rd=1, js=153, timex=30, override=0.", L"Warning", MB_OK | MB_ICONWARNING);
    }
}

void createsk(int x, int y) {
    FILE* fp = fopen("config.js", "w+");
    if (fp) {
        fprintf(fp, "const config = { width: %d, height: %d };\n", x, y);
        fclose(fp);
    }
}

void createindex() {
    char filename[256];
    GenerateRandomFilename(filename, sizeof(filename));

    std::wstring exeDir = GetExecutableDirectory();
    std::wstring sketchPath = exeDir + L"\\" + std::wstring(filename, filename + strlen(filename));
    if (_waccess(sketchPath.c_str(), 0) != 0) {
        MessageBox(NULL, (L"Sketch file " + sketchPath + L" not found!").c_str(), L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    FILE* fp = fopen("index.html", "w+");
    if (fp) {
        fprintf(fp, "<!DOCTYPE html>\n<html lang=\"en\">\n");
        fprintf(fp, "<head>\n<meta charset=\"UTF-8\">\n");
        fprintf(fp, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
        fprintf(fp, "<title>p5.js Example</title>\n");
        fprintf(fp, "<style>\nbody {\npadding: 0;\nmargin: 0;\noverflow: hidden;\nbackground-color: #1b1b1b;\n}\ncanvas {\ndisplay: block;\n}\n</style>\n");
        fprintf(fp, "<script src=\"p5.min.js\"></script>\n");
        fprintf(fp, "<script src=\"config.js\"></script>\n");
        fprintf(fp, "<script src=\"%s\"></script>\n", filename);
        fprintf(fp, "</head>\n<body>\n<main>\n</main>\n");
        fprintf(fp, "<script>document.body.style.pointerEvents = 'none';</script>\n");
        fprintf(fp, "</body>\n</html>\n");
        fclose(fp);
    }
}

void MakeWindowFullscreen(HWND hwnd) {
    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);
    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_APPWINDOW);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, screenX, screenY, SWP_FRAMECHANGED);
    createsk(screenX, screenY);
    createindex();
}

void DisableMouse(HWND hwnd) {
    ShowCursor(FALSE);
    RECT rect;
    GetClientRect(hwnd, &rect);
    MapWindowPoints(hwnd, NULL, (POINT*)&rect, 2);
    ClipCursor(&rect);
}

void EnableMouse(HWND hwnd) {
    ShowCursor(TRUE);
    ClipCursor(NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        if (webViewController) {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            webViewController->put_Bounds(bounds);
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
            break;
        }
        if (wParam == VK_F1) {
            DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(TEXT_EDITOR), hwnd, TextEditorProc);
            break;
        }
        if (wParam == VK_SPACE) {
            if (webView) {
                createindex();
                SetCurrentDirectoryW(exePath);
                std::wstring exeDir = GetExecutableDirectory();
                std::wstring htmlPath = L"file:///" + exeDir + L"/index.html?t=" + std::to_wstring(std::time(nullptr));
                webView->Navigate(htmlPath.c_str());
                webView->Reload();
            }
            break;
        }
        break;

    case WM_TIMER:
        if (webView) {
            createindex();
            SetCurrentDirectoryW(exePath);
            std::wstring exeDir = GetExecutableDirectory();
            std::wstring htmlPath = L"file:///" + exeDir + L"/index.html?t=" + std::to_wstring(std::time(nullptr));
            webView->Navigate(htmlPath.c_str());
            webView->Reload();
        }
        return 0;

    case WM_DESTROY:
        if (webViewController) webViewController->Close();
        webViewController.reset();
        webView.reset();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void InitWebView2(HWND hwnd) {
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hwnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (controller) {
                            webViewController = controller;
                            webViewController->get_CoreWebView2(&webView);
                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            webViewController->put_Bounds(bounds);
                            wil::com_ptr<ICoreWebView2Settings> settings;
                            webView->get_Settings(&settings);
                            settings->put_IsBuiltInErrorPageEnabled(FALSE);
                            settings->put_AreDefaultContextMenusEnabled(FALSE);
                            settings->put_IsScriptEnabled(TRUE);
                            settings->put_IsZoomControlEnabled(FALSE);
                            std::wstring exeDir = GetExecutableDirectory();
                            std::wstring htmlPath = L"file:///" + exeDir + L"/index.html";
                            webView->Navigate(htmlPath.c_str());
                            SetTimer(hwnd, 1, timex * 1000, NULL);
                        }
                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"WebView2 Example";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        PathRemoveFileSpecW(exePath);
        SetCurrentDirectoryW(exePath);
    }

    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Wzor.org", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    MakeWindowFullscreen(hwnd);
    // DisableMouse(hwnd); // Uncomment if mouse should be disabled
    SetFocus(hwnd);
    ShowWindow(hwnd, nCmdShow);

    load();
    InitWebView2(hwnd);

    MSG msg = {};
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (bRet == -1) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) {
            EnableMouse(hwnd);
            break;
        }
    }

    EnableMouse(hwnd);
    return (int)msg.wParam;
}

INT_PTR CALLBACK TextEditorProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SAVE) {
            SaveToFile(hDlg);
            return (INT_PTR)TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

void SaveToFile(HWND hDlg) {
    OPENFILENAME ofn;
    wchar_t fileName[MAX_PATH] = L"";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = L"JavaScript Files\0*.js\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"js";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        HWND hEdit = GetDlgItem(hDlg, IDC_TEXTBOX);
        int length = GetWindowTextLength(hEdit);
        wchar_t* buffer = new wchar_t[length + 1];
        GetWindowText(hEdit, buffer, length + 1);
        FILE* file;
        _wfopen_s(&file, fileName, L"w, ccs=UTF-8");
        if (file) {
            fwprintf(file, L"%s", buffer);
            fclose(file);
        }
        delete[] buffer;
    }
}