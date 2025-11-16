#define UNICODE
#define NOMINMAX
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")

using namespace std;

enum : int {
    IDC_COMBO_PORTS = 1001,
    IDC_BTN_REFRESH = 1002,
    IDC_BTN_CONNECT = 1003,
    IDC_EDIT_TEXT = 1004,
    IDC_BTN_SEND = 1005
};

HINSTANCE hInst;
HWND hWndMain, hCbPorts, hBtnRefresh, hBtnConnect, hEditText, hBtnSend;
HANDLE hSerial = INVALID_HANDLE_VALUE;
bool connected = false;

void MsgBox(const wstring& msg, UINT icon = MB_ICONINFORMATION) {
    MessageBoxW(hWndMain, msg.c_str(), L"USB Text Sender", MB_OK | icon);
}

vector<wstring> EnumComPorts() {
    vector<wstring> ports;
    HDEVINFO info = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_COMPORT, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) {
        for (int i = 1; i <= 16; i++) {
            wstringstream ss; ss << L"COM" << i;
            ports.push_back(ss.str());
        }
        return ports;
    }

    SP_DEVICE_INTERFACE_DATA ifData{ sizeof(SP_DEVICE_INTERFACE_DATA) };
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(info, nullptr, &GUID_DEVINTERFACE_COMPORT, i, &ifData); ++i) {
        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(info, &ifData, nullptr, 0, &required, nullptr);
        vector<BYTE> buf(required);
        auto detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buf.data();
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (SetupDiGetDeviceInterfaceDetailW(info, &ifData, detail, required, nullptr, nullptr)) {
            SP_DEVINFO_DATA devInfo{ sizeof(SP_DEVINFO_DATA) };
            if (SetupDiEnumDeviceInfo(info, i, &devInfo)) {
                wchar_t name[256];
                if (SetupDiGetDeviceRegistryPropertyW(info, &devInfo, SPDRP_FRIENDLYNAME, nullptr, (PBYTE)name, sizeof(name), nullptr)) {
                    wstring friendly = name;
                    size_t p1 = friendly.find(L"(COM");
                    size_t p2 = friendly.find(L")", p1);
                    if (p1 != wstring::npos && p2 != wstring::npos) {
                        wstring com = friendly.substr(p1 + 1, p2 - (p1 + 1)); // COMx
                        ports.push_back(com);
                    }
                }
            }
        }
    }
    SetupDiDestroyDeviceInfoList(info);
    sort(ports.begin(), ports.end());
    ports.erase(unique(ports.begin(), ports.end()), ports.end());
    return ports;
}

void PopulatePorts() {
    SendMessageW(hCbPorts, CB_RESETCONTENT, 0, 0);
    for (auto& p : EnumComPorts())
        SendMessageW(hCbPorts, CB_ADDSTRING, 0, (LPARAM)p.c_str());
    SendMessageW(hCbPorts, CB_SETCURSEL, 0, 0);
}

wstring GetSelectedPort() {
    int idx = (int)SendMessageW(hCbPorts, CB_GETCURSEL, 0, 0);
    if (idx < 0) return L"";
    wchar_t buf[64];
    SendMessageW(hCbPorts, CB_GETLBTEXT, idx, (LPARAM)buf);
    return buf;
}

bool OpenSerial(const wstring& port) {
    if (connected) return true;
    wstring path = L"\\\\.\\" + port;
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) { MsgBox(L"Cannot open port.", MB_ICONERROR); return false; }

    DCB dcb{ sizeof(DCB) };
    GetCommState(h, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(h, &dcb);

    COMMTIMEOUTS to{ 0,0,50,0,200 };
    SetCommTimeouts(h, &to);
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    hSerial = h;
    connected = true;
    SetWindowTextW(hBtnConnect, L"Disconnect");
    return true;
}

void CloseSerial() {
    if (hSerial != INVALID_HANDLE_VALUE) CloseHandle(hSerial);
    hSerial = INVALID_HANDLE_VALUE;
    connected = false;
    SetWindowTextW(hBtnConnect, L"Connect");
}

bool SendText(const wstring& text) {
    if (!connected) { MsgBox(L"Not connected.", MB_ICONWARNING); return false; }

    int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), (LPSTR)utf8.data(), len, nullptr, nullptr);
    utf8.push_back('\n');

    DWORD written;
    if (!WriteFile(hSerial, utf8.data(), (DWORD)utf8.size(), &written, nullptr)) {
        MsgBox(L"Write failed.", MB_ICONERROR);
        return false;
    }
    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int margin = 10, rowH = 28, btnW = 100;
        hCbPorts = CreateWindowW(L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            margin, margin, 180, 200, hWnd, (HMENU)IDC_COMBO_PORTS, hInst, nullptr);
        hBtnRefresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE,
            margin + 190, margin, btnW, rowH, hWnd, (HMENU)IDC_BTN_REFRESH, hInst, nullptr);
        hBtnConnect = CreateWindowW(L"BUTTON", L"Connect", WS_CHILD | WS_VISIBLE,
            margin + 300, margin, btnW, rowH, hWnd, (HMENU)IDC_BTN_CONNECT, hInst, nullptr);
        hEditText = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL,
            margin, margin + rowH + 10, 460, 200, hWnd, (HMENU)IDC_EDIT_TEXT, hInst, nullptr);
        hBtnSend = CreateWindowW(L"BUTTON", L"Send", WS_CHILD | WS_VISIBLE,
            margin, margin + rowH + 220, 100, rowH, hWnd, (HMENU)IDC_BTN_SEND, hInst, nullptr);
        PopulatePorts();
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_REFRESH && HIWORD(wParam) == BN_CLICKED) PopulatePorts();
        else if (id == IDC_BTN_CONNECT && HIWORD(wParam) == BN_CLICKED) {
            if (connected) { CloseSerial(); MsgBox(L"Disconnected."); }
            else {
                wstring sel = GetSelectedPort();
                if (sel.empty()) { MsgBox(L"No port selected.", MB_ICONWARNING); return 0; }
                if (OpenSerial(sel)) { MsgBox((L"Connected to " + sel).c_str()); }
            }
        }
        else if (id == IDC_BTN_SEND && HIWORD(wParam) == BN_CLICKED) {
            wchar_t buf[2048]; GetWindowTextW(hEditText, buf, 2048);
            if (wcslen(buf) == 0) MsgBox(L"Nothing to send.", MB_ICONWARNING);
            else if (SendText(buf)) MsgBox(L"Sent successfully.");
        }
        break;
    }
    case WM_DESTROY: CloseSerial(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    hInst = hInstance;
    WNDCLASSW wc{ };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"UsbTextSender";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    hWndMain = CreateWindowW(L"UsbTextSender", L"Text Sender",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 360,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
