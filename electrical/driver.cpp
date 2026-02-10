#define UNICODE
#define NOMINMAX
#include <windows.h>
#include <unknwn.h>
#include <windowsx.h>
#include <setupapi.h>
#include <filesystem>
#include <devguid.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <shlwapi.h>
#include <shcore.h>
#include <wincodec.h>
#include <Windows.Graphics.Capture.Interop.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

// WinRT interop header from Windows SDK:
#include <windows.graphics.directx.direct3d11.interop.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <DispatcherQueue.h>          // Windows SDK
#include <winrt/Windows.System.h>     // for DispatcherQueue (optional but useful)


#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "windowscodecs.lib")




using namespace std;



enum : int {
    IDC_COMBO_PORTS = 1001,
    IDC_BTN_REFRESH = 1002,
    IDC_BTN_CONNECT = 1003,
    IDC_EDIT_TEXT = 1004,
    IDC_BTN_SEND = 1005,
    IDC_BTN_OCR_REGION = 1006,
    IDC_BTN_OCR_FULLSCREEN = 1007
};




HINSTANCE hInst;
HWND hWndMain, hCbPorts, hBtnRefresh, hBtnConnect, hEditText, hBtnSend;
HANDLE hSerial = INVALID_HANDLE_VALUE;
bool connected = false;

static winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice g_d3dDevice{ nullptr };
static winrt::Windows::System::DispatcherQueueController g_dqController{ nullptr };








void MsgBox(const wstring& msg, UINT icon = MB_ICONINFORMATION) {
    MessageBoxW(hWndMain, msg.c_str(), L"USB Text Sender", MB_OK | icon);
}







// From Windows.Graphics.Capture.Interop (we declare them ourselves to avoid extra headers)
/*struct __declspec(uuid("79C3F95B-31F7-4EC2-A464-632EF5D30760"))
    IGraphicsCaptureItemInterop : IUnknown {
    virtual HRESULT __stdcall CreateForWindow(HWND window, REFIID iid, void** result) = 0;
    virtual HRESULT __stdcall CreateForMonitor(HMONITOR monitor, REFIID iid, void** result) = 0;
};*/

// Create IDirect3DDevice from DXGI device (from windows.graphics.directx.direct3d11.interop.h)
extern "C" HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(
    ::IDXGIDevice* dxgiDevice,
    ::IInspectable** graphicsDevice
);


static winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice CreateD3DDevice() {
    winrt::com_ptr<ID3D11Device> d3dDevice;
    winrt::com_ptr<ID3D11DeviceContext> ctx;

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL chosen{};
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        flags, levels, ARRAYSIZE(levels),
        D3D11_SDK_VERSION,
        d3dDevice.put(), &chosen, ctx.put()
    );
    if (FAILED(hr)) {
        // fallback: WARP
        hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
            flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION,
            d3dDevice.put(), &chosen, ctx.put()
        );
        winrt::check_hresult(hr);
    }

    winrt::com_ptr<IDXGIDevice> dxgi;
    winrt::check_hresult(d3dDevice->QueryInterface(IID_PPV_ARGS(dxgi.put())));

    winrt::com_ptr<IInspectable> insp;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(), insp.put()));

    return insp.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

static winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateItemForMonitor(HMONITOR mon)
{
    namespace WGC = winrt::Windows::Graphics::Capture;

    auto factory = winrt::get_activation_factory<WGC::GraphicsCaptureItem>();
    auto interop = factory.as<IGraphicsCaptureItemInterop>();

    WGC::GraphicsCaptureItem item{ nullptr };
    winrt::check_hresult(
        interop->CreateForMonitor(
            mon,
            winrt::guid_of<WGC::GraphicsCaptureItem>(),
            winrt::put_abi(item)
        )
    );
    return item;
}

static HMONITOR GetAppMonitor(HWND hwnd) {
    return MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
}

static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Graphics::Imaging::SoftwareBitmap>
CaptureOneFrameAsync(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item)
{
    using namespace winrt;
    using namespace winrt::Windows::Graphics::Capture;
    using namespace winrt::Windows::Graphics::DirectX;
    using namespace winrt::Windows::Graphics::Imaging;

    if (!GraphicsCaptureSession::IsSupported()) co_return nullptr;

    auto size = item.Size();
    //MsgBox(L"[DEBUG] Item size = " + to_wstring(size.Width) + L"x" + to_wstring(size.Height));
    if (size.Width <= 0 || size.Height <= 0) co_return nullptr;

    // IMPORTANT: Free-threaded frame pool (no DispatcherQueue dependency)
    auto framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
        g_d3dDevice,
        DirectXPixelFormat::B8G8R8A8UIntNormalized,
        1,
        size
    );

    auto session = framePool.CreateCaptureSession(item);

    winrt::handle frameReady(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    Direct3D11CaptureFrame frame{ nullptr };

    auto token = framePool.FrameArrived([&](auto const& sender, auto const&) {
        frame = sender.TryGetNextFrame();
        SetEvent(frameReady.get());
        });

    session.StartCapture();

    // Give it longer; 2s is sometimes too short on first permission/capture
    DWORD waitRes = WaitForSingleObject(frameReady.get(), 5000);

    framePool.FrameArrived(token);
    session.Close();
    framePool.Close();

    if (waitRes != WAIT_OBJECT_0 || !frame) co_return nullptr;

    // Convert captured surface -> SoftwareBitmap
    auto sb = co_await SoftwareBitmap::CreateCopyFromSurfaceAsync(frame.Surface());
    sb = SoftwareBitmap::Convert(sb, BitmapPixelFormat::Bgra8, BitmapAlphaMode::Ignore);
    co_return sb;
}

static winrt::Windows::Graphics::Imaging::SoftwareBitmap CropSoftwareBitmap(
    winrt::Windows::Graphics::Imaging::SoftwareBitmap const& src,
    RECT rPx,
    int /*unused*/, int /*unused*/
) {
    using namespace winrt::Windows::Graphics::Imaging;

    int x = rPx.left;
    int y = rPx.top;
    int w = rPx.right - rPx.left;
    int h = rPx.bottom - rPx.top;

    if (w <= 0 || h <= 0) return nullptr;

    x = max(0, min(x, src.PixelWidth() - 1));
    y = max(0, min(y, src.PixelHeight() - 1));
    w = min(w, src.PixelWidth() - x);
    h = min(h, src.PixelHeight() - y);

    auto dst = SoftwareBitmap(BitmapPixelFormat::Bgra8, w, h, BitmapAlphaMode::Ignore);

    auto srcBuf = src.LockBuffer(BitmapBufferAccessMode::Read);
    auto dstBuf = dst.LockBuffer(BitmapBufferAccessMode::Write);

    auto srcPlane = srcBuf.GetPlaneDescription(0);
    auto dstPlane = dstBuf.GetPlaneDescription(0);

    struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")) IMemoryBufferByteAccess : IUnknown {
        virtual HRESULT __stdcall GetBuffer(uint8_t** buffer, uint32_t* capacity) = 0;
    };

    auto srcRef = srcBuf.CreateReference();
    auto dstRef = dstBuf.CreateReference();

    uint8_t* srcPtr = nullptr; uint32_t srcCap = 0;
    uint8_t* dstPtr = nullptr; uint32_t dstCap = 0;

    winrt::check_hresult(srcRef.as<IMemoryBufferByteAccess>()->GetBuffer(&srcPtr, &srcCap));
    winrt::check_hresult(dstRef.as<IMemoryBufferByteAccess>()->GetBuffer(&dstPtr, &dstCap));

    const int bpp = 4;
    const size_t rowBytes = (size_t)w * bpp;

    for (int row = 0; row < h; ++row) {
        auto* s = srcPtr + (size_t)(y + row) * srcPlane.Stride + (size_t)x * bpp;
        auto* d = dstPtr + (size_t)row * dstPlane.Stride;
        memcpy(d, s, rowBytes);
    }
    return dst;
}

static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring>OcrBitmapAsync(winrt::Windows::Graphics::Imaging::SoftwareBitmap sb) {
    using namespace winrt;
    using namespace winrt::Windows::Media::Ocr;
    using namespace winrt::Windows::Graphics::Imaging;

    if (!sb) co_return L"";

    // Optional: Gray8 often improves OCR
    sb = SoftwareBitmap::Convert(sb, BitmapPixelFormat::Gray8);

    auto engine = OcrEngine::TryCreateFromUserProfileLanguages();
    if (!engine) co_return L"";

    auto res = co_await engine.RecognizeAsync(sb);
    co_return res ? res.Text() : L"";
}

static std::wstring GetTempPathFile(const wchar_t* name) {
    wchar_t tmp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmp);
    return std::wstring(tmp) + name;
}

// If file doesn't exist yet, create it
static void EnsureEmptyFile(std::wstring const& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
}

static std::wstring TempFilePath(const wchar_t* name) {
    wchar_t tmp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmp);
    return std::wstring(tmp) + name;
}

static std::wstring TempFile(const wchar_t* name)
{
    wchar_t path[MAX_PATH]{};
    GetTempPathW(MAX_PATH, path);
    return std::wstring(path) + name;
}

// Save a SoftwareBitmap (Bgra8) as BMP using WIC.
// Returns true if successful.
static bool SaveSoftwareBitmapAsBMP(
    const winrt::Windows::Graphics::Imaging::SoftwareBitmap& sb,
    const std::wstring& path,
    std::wstring& outErr)
{
    outErr.clear();
    using namespace winrt;
    using namespace winrt::Windows::Graphics::Imaging;

    if (!sb) { outErr = L"SoftwareBitmap is null."; return false; }

    // Force BGRA8
    SoftwareBitmap src = sb;
    if (src.BitmapPixelFormat() != BitmapPixelFormat::Bgra8) {
        src = SoftwareBitmap::Convert(src, BitmapPixelFormat::Bgra8, BitmapAlphaMode::Ignore);
    }

    int w = src.PixelWidth();
    int h = src.PixelHeight();
    if (w <= 0 || h <= 0) { outErr = L"Invalid bitmap size."; return false; }

    auto buf = src.LockBuffer(BitmapBufferAccessMode::Read);
    auto desc = buf.GetPlaneDescription(0);
    int stride = desc.Stride;

    auto ref = buf.CreateReference();

    struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")) IMemoryBufferByteAccess : IUnknown {
        virtual HRESULT __stdcall GetBuffer(uint8_t** buffer, uint32_t* capacity) = 0;
    };

    uint8_t* p = nullptr;
    uint32_t cap = 0;
    winrt::check_hresult(ref.as<IMemoryBufferByteAccess>()->GetBuffer(&p, &cap));

    // Convert to tight-packed BGRA (w*h*4), because stride may be larger than w*4
    std::vector<uint8_t> bgra((size_t)w * (size_t)h * 4);
    for (int y = 0; y < h; ++y) {
        memcpy(bgra.data() + (size_t)y * (size_t)w * 4, p + (size_t)y * (size_t)stride, (size_t)w * 4);
    }

    BITMAPFILEHEADER bfh{};
    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = w;
    bih.biHeight = -h; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    DWORD imageSize = (DWORD)(w * h * 4);
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        outErr = L"CreateFileW failed. Error " + std::to_wstring(GetLastError());
        return false;
    }

    DWORD written = 0;
    BOOL ok = TRUE;
    ok &= WriteFile(f, &bfh, sizeof(bfh), &written, nullptr);
    ok &= WriteFile(f, &bih, sizeof(bih), &written, nullptr);
    ok &= WriteFile(f, bgra.data(), imageSize, &written, nullptr);
    CloseHandle(f);

    if (!ok) {
        outErr = L"WriteFile failed. Error " + std::to_wstring(GetLastError());
        return false;
    }
    return true;
}

static void PostToEdit(const std::wstring& s)
{
    auto* p = new std::wstring(s);
    if (!PostMessageW(hWndMain, WM_APP + 1, 0, (LPARAM)p)) delete p;
}

static std::wstring HrToStr(winrt::hresult hr)
{
    wchar_t* buf = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, (DWORD)hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&buf, 0, nullptr);
    std::wstring msg = buf ? buf : L"(no system message)";
    if (buf) LocalFree(buf);
    return msg;
}















static void AppendToEdit(HWND hEdit, const std::wstring& line) {
    if (!IsWindow(hEdit)) return;

    // Move caret to end
    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);

    std::wstring s = line;
    s += L"\r\n";
    SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}

wstring GetLastErrorMessage(DWORD err) {
    wchar_t* msgBuf = nullptr;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&msgBuf,
        0, nullptr
    );

    wstring msg = msgBuf ? msgBuf : L"(Unknown error)";
    LocalFree(msgBuf);
    return msg;
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

static std::wstring GetTempBmpPath() {
    wchar_t tmp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmp);
    // e.g. C:\Users\Willi\AppData\Local\Temp\ocr_debug.bmp
    return std::wstring(tmp) + L"ocr_debug.bmp";
}

static bool SaveBGRAasBMP(const std::wstring& path,
    const std::vector<uint8_t>& bgra, int w, int h,
    std::wstring& outErr)
{
    outErr.clear();

    if (w <= 0 || h <= 0 || bgra.size() < (size_t)w * (size_t)h * 4) {
        outErr = L"Invalid image buffer.";
        return false;
    }

    BITMAPFILEHEADER bfh{};
    BITMAPINFOHEADER bih{};

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = w;
    bih.biHeight = -h; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    DWORD imageSize = (DWORD)(w * h * 4);
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    HANDLE f = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        outErr = L"CreateFileW failed. Error " + std::to_wstring(err);
        return false;
    }

    DWORD written = 0;
    BOOL ok = TRUE;
    ok &= WriteFile(f, &bfh, sizeof(bfh), &written, nullptr);
    ok &= WriteFile(f, &bih, sizeof(bih), &written, nullptr);
    ok &= WriteFile(f, bgra.data(), imageSize, &written, nullptr);
    CloseHandle(f);

    if (!ok) {
        DWORD err = GetLastError();
        outErr = L"WriteFile failed. Error " + std::to_wstring(err);
        return false;
    }

    return true;
}

void LoadTextFile(const std::wstring& filePath) {
    try {
        std::ifstream file(std::filesystem::path(filePath), std::ios::binary);
        if (!file) { MsgBox(L"Failed to open the file.", MB_ICONERROR); return; }

        std::ostringstream oss;
        oss << file.rdbuf();
        std::string bytes = oss.str();


        if (!bytes.empty()) {
            int required = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
            std::wstring text;
            if (required > 0) {
                text.resize(required);
                MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), text.data(), required);
            }
            SetWindowTextW(hEditText, text.c_str());
        } else {
            SetWindowTextW(hEditText, L"");
        }
    } catch (...) {
        MsgBox(L"Failed to read file (exception).", MB_ICONERROR);
    }
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
    int len = (int)SendMessageW(hCbPorts, CB_GETLBTEXTLEN, idx, 0);
    if (len < 0) return L"";
    std::wstring buf;
    buf.resize(len + 1);
    SendMessageW(hCbPorts, CB_GETLBTEXT, idx, (LPARAM)buf.data());
    buf.resize(len);
    return buf;
}

bool OpenSerial(const wstring& port) {
    if (connected) return true;

    wstring path = L"\\\\.\\" + port;
    HANDLE h = CreateFileW(path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        MsgBox(L"Cannot open port.", MB_ICONERROR);
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        MsgBox(L"GetCommState failed.", MB_ICONERROR);
        return false;
    }

    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    // enable DTR/RTS — many Arduino-style boards expect this
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE; //111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111

    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        MsgBox(L"SetCommState failed.", MB_ICONERROR);
        return false;
    }

    COMMTIMEOUTS to{};
    to.ReadIntervalTimeout = MAXDWORD;
    to.ReadTotalTimeoutMultiplier = 50;
    to.ReadTotalTimeoutConstant = 50;
    to.WriteTotalTimeoutMultiplier = 50;
    to.WriteTotalTimeoutConstant = 2000;

    if (!SetCommTimeouts(h, &to)) {
        CloseHandle(h);
        MsgBox(L"SetCommTimeouts failed.", MB_ICONERROR);
        return false;
    }

    // Clear any junk in buffers
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

bool SendText(const std::wstring& text) {
    if (!connected) { MsgBox(L"Not connected.", MB_ICONWARNING); return false; }

    int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8;
    if (len > 0) {
        utf8.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), utf8.data(), len, nullptr, nullptr);
    }
    utf8.push_back('\n');

    DWORD written = 0;
    if (!WriteFile(hSerial, utf8.data(), (DWORD)utf8.size(), &written, nullptr)) {
        DWORD err = GetLastError();
        wstring msg = L"WriteFile failed.\nError " + to_wstring(err) + L": " + GetLastErrorMessage(err);
        MsgBox(msg, MB_ICONERROR);
        return false;
    }
    if (written != utf8.size()) {
        MsgBox(L"Partial write to serial port.", MB_ICONWARNING);
        return false;
    }
    return true;
    
}







struct RegionSelectState {
    bool selecting = false;
    POINT start{};
    POINT end{};
    RECT  result{};
    bool  done = false;
    bool  cancelled = false;
};

static void NormalizeRect(RECT& r) {
    if (r.left > r.right) std::swap(r.left, r.right);
    if (r.top > r.bottom) std::swap(r.top, r.bottom);
}

static LRESULT CALLBACK RegionSelectProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* st = reinterpret_cast<RegionSelectState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg) {
    case WM_CREATE: {
        auto cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        st = reinterpret_cast<RegionSelectState*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)st);
        SetCapture(hWnd);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && st) {
            st->cancelled = true;
            st->done = true;
            ReleaseCapture();
            DestroyWindow(hWnd);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (st) {
            st->selecting = true;
            st->start = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            st->end = st->start;
            InvalidateRect(hWnd, nullptr, TRUE);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (st && st->selecting) {
            st->end = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            InvalidateRect(hWnd, nullptr, TRUE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (st && st->selecting) {
            st->selecting = false;
            st->end = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            RECT r{ st->start.x, st->start.y, st->end.x, st->end.y };
            NormalizeRect(r);
            st->result = r;

            st->done = true;
            ReleaseCapture();
            DestroyWindow(hWnd);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Darken background a bit
        RECT client{};
        GetClientRect(hWnd, &client);
        HBRUSH b = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &client, b);
        DeleteObject(b);

        // Draw selection rectangle
        if (st) {
            RECT r{ st->start.x, st->start.y, st->end.x, st->end.y };
            NormalizeRect(r);

            HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
            Rectangle(hdc, r.left, r.top, r.right, r.bottom);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool SelectScreenRegion(HWND owner, RECT& outRect) {
    RegionSelectState st{};

    const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    WNDCLASSW wc{};
    wc.lpfnWndProc = RegionSelectProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"RegionSelectOverlay";
    wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    ATOM reg = RegisterClassW(&wc);
    if (reg == 0) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    HWND overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName,
        L"",
        WS_POPUP,
        vx, vy, vw, vh,
        owner,
        nullptr,
        wc.hInstance,
        &st
    );

    if (!overlay) {
        return false;
    }

    if (!SetLayeredWindowAttributes(overlay, 0, (BYTE)80, LWA_ALPHA)) {
    }

    ShowWindow(overlay, SW_SHOW);
    UpdateWindow(overlay);

    // Modal-ish loop
    MSG msg;
    while (!st.done && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (st.cancelled) return false;

    // Convert overlay-relative coords to screen coords
    outRect = st.result;
    OffsetRect(&outRect, vx, vy);
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

        CreateWindowW(L"BUTTON", L"OCR (region)", WS_CHILD | WS_VISIBLE,
            margin + 110, margin + rowH + 220, 120, rowH, hWnd, (HMENU)IDC_BTN_OCR_REGION, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"OCR (full)", WS_CHILD | WS_VISIBLE,
            margin + 240, margin + rowH + 220, 120, rowH, hWnd, (HMENU)IDC_BTN_OCR_FULLSCREEN, hInst, nullptr);

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
        else if (id == IDC_BTN_OCR_FULLSCREEN && HIWORD(wParam) == BN_CLICKED)
        {
            winrt::fire_and_forget([]() -> winrt::fire_and_forget {
                try {
                    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
                        PostToEdit(L"[OCR DEBUG] Windows.Graphics.Capture is not supported on this system.");
                        co_return;
                    }

                    HMONITOR mon = MonitorFromWindow(hWndMain, MONITOR_DEFAULTTONEAREST);
                    auto item = CreateItemForMonitor(mon);
                    if (!item) {
                        PostToEdit(L"[OCR DEBUG] CreateItemForMonitor returned null.");
                        co_return;
                    }

                    auto sb = co_await CaptureOneFrameAsync(item);
                    if (!sb) {
                        PostToEdit(L"[OCR DEBUG] CaptureOneFrameAsync returned null (no frame).");
                        co_return;
                    }

                    auto text = co_await OcrBitmapAsync(sb);

                    std::wstring t = text.c_str();
                    // trim
                    const wchar_t* ws = L" \t\r\n";
                    size_t a = t.find_first_not_of(ws), b = t.find_last_not_of(ws);
                    if (a == std::wstring::npos) t.clear(); else t = t.substr(a, b - a + 1);

                    if (t.empty()) PostToEdit(L"[OCR] (empty result)");
                    else PostToEdit(t);
                }
                catch (winrt::hresult_error const& e) {
                    std::wstring msg = L"[OCR DEBUG] hresult_error 0x" + std::to_wstring((uint32_t)e.code().value)
                        + L"\n" + HrToStr(e.code())
                        + L"\n" + std::wstring(e.message().c_str());
                    PostToEdit(msg);
                }
                catch (std::exception const& e) {
                    std::string s = e.what();
                    std::wstring ws(s.begin(), s.end());
                    PostToEdit(L"[OCR DEBUG] std::exception: " + ws);
                }
                catch (...) {
                    PostToEdit(L"[OCR DEBUG] Unknown exception.");
                }
                co_return;
                }());
        }
        else if (id == IDC_BTN_OCR_REGION && HIWORD(wParam) == BN_CLICKED)
        {
            RECT selected{};
            if (!SelectScreenRegion(hWndMain, selected)) return 0;

            winrt::fire_and_forget([selected]() -> winrt::fire_and_forget {
                try {
                    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
                        PostToEdit(L"[OCR DEBUG] Windows.Graphics.Capture is not supported on this system.");
                        co_return;
                    }

                    // IMPORTANT: make a local mutable copy (fixes “rc = clipped” compile errors)
                    RECT rc = selected;

                    // Pick monitor based on selection point
                    POINT p{ rc.left + 1, rc.top + 1 };
                    HMONITOR mon = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);

                    MONITORINFO mi{};
                    mi.cbSize = sizeof(mi);
                    if (!GetMonitorInfoW(mon, &mi)) {
                        PostToEdit(L"[OCR DEBUG] GetMonitorInfoW failed.");
                        co_return;
                    }

                    // Clip selection to that monitor
                    RECT clipped{};
                    if (!IntersectRect(&clipped, &rc, &mi.rcMonitor)) {
                        PostToEdit(L"[OCR DEBUG] Selected region not on captured monitor.");
                        co_return;
                    }
                    rc = clipped;

                    int screenLeft = mi.rcMonitor.left;
                    int screenTop = mi.rcMonitor.top;

                    auto item = CreateItemForMonitor(mon);
                    if (!item) {
                        PostToEdit(L"[OCR DEBUG] CreateItemForMonitor returned null.");
                        co_return;
                    }

                    auto full = co_await CaptureOneFrameAsync(item);
                    if (!full) {
                        PostToEdit(L"[OCR DEBUG] CaptureOneFrameAsync returned null (no frame).");
                        co_return;
                    }

                    auto cropped = CropSoftwareBitmap(full, rc, screenLeft, screenTop);
                    if (!cropped) {
                        PostToEdit(L"[OCR DEBUG] CropSoftwareBitmap returned null.");
                        co_return;
                    }

                    auto text = co_await OcrBitmapAsync(cropped);

                    std::wstring t = text.c_str();
                    // trim
                    const wchar_t* ws = L" \t\r\n";
                    size_t a = t.find_first_not_of(ws), b = t.find_last_not_of(ws);
                    if (a == std::wstring::npos) t.clear(); else t = t.substr(a, b - a + 1);

                    if (t.empty()) PostToEdit(L"[OCR] (empty result)");
                    else PostToEdit(t);
                }
                catch (winrt::hresult_error const& e) {
                    std::wstring msg = L"[OCR DEBUG] hresult_error 0x" + std::to_wstring((uint32_t)e.code().value)
                        + L"\n" + HrToStr(e.code())
                        + L"\n" + std::wstring(e.message().c_str());
                    PostToEdit(msg);
                }
                catch (...) {
                    PostToEdit(L"[OCR DEBUG] Unknown exception.");
                }
                co_return;
                }());
        }

        break;
    }
    case WM_APP + 1: {
        auto* p = reinterpret_cast<std::wstring*>(lParam);
        if (p) {
            std::wstring t = *p;
            delete p;

            // Trim whitespace
            const wchar_t* ws = L" \t\r\n";
            size_t a = t.find_first_not_of(ws);
            size_t b = t.find_last_not_of(ws);
            if (a == std::wstring::npos) t.clear();
            else t = t.substr(a, b - a + 1);

            if (t.empty()) {
                AppendToEdit(hEditText, L"[OCR] (empty result)");
            }
            else {
                AppendToEdit(hEditText, t);
            }
        }
        return 0;
    }
    case WM_DESTROY: CloseSerial(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void EnsureDispatcherQueue() {
    // If the current thread already has a DispatcherQueue, we're good.
    if (winrt::Windows::System::DispatcherQueue::GetForCurrentThread()) return;

    DispatcherQueueOptions options{};
    options.dwSize = sizeof(options);
    options.threadType = DQTYPE_THREAD_CURRENT;
    options.apartmentType = DQTAT_COM_STA;

    ABI::Windows::System::IDispatcherQueueController* controller = nullptr;
    HRESULT hr = CreateDispatcherQueueController(options, &controller);
    winrt::check_hresult(hr);

    g_dqController = winrt::Windows::System::DispatcherQueueController{ controller, winrt::take_ownership_from_abi };
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    SetProcessDPIAware();
    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    EnsureDispatcherQueue();
    g_d3dDevice = CreateD3DDevice();

    hInst = hInstance;
    WNDCLASSW wc{ };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"UsbTextSender";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    hWndMain = CreateWindowW(L"UsbTextSender", L"Text Sender v1.1.0",
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
    winrt::uninit_apartment();
    return (int)msg.wParam;
}
