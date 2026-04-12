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
#include <commdlg.h>
#include <wincodec.h>
#include <Windows.Graphics.Capture.Interop.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Data.Pdf.h>
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


#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#include <string>
#include <filesystem>


#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "python310.lib")




using namespace std;



enum : int {
    IDC_COMBO_PORTS = 1001,
    IDC_BTN_REFRESH = 1002,
    IDC_BTN_CONNECT = 1003,
    IDC_EDIT_TEXT = 1004,
    IDC_BTN_SEND = 1005,
    IDC_BTN_OCR_REGION = 1006,
    IDC_BTN_OCR_FULLSCREEN = 1007,
    IDC_BTN_OPEN = 1008,
    IDC_BTN_SAVE = 1009,
    IDC_RADIO_REGULAR = 1010,
    IDC_RADIO_SHORTHAND = 1011
};




HINSTANCE hInst;
HWND hWndMain, hCbPorts, hBtnRefresh, hBtnConnect, hEditText, hBtnSend, hBtnRegion, hBtnFull, hBtnOpen, hBtnSave, hRadioRegular, hRadioShorthand;
HANDLE hSerial = INVALID_HANDLE_VALUE;
bool connected = false;
bool g_useShorthand = false;
WNDPROC OriginalEditProc;

static winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice g_d3dDevice{ nullptr };
static winrt::Windows::System::DispatcherQueueController g_dqController{ nullptr };

static std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring p = path;
    size_t pos = p.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        p.resize(pos);
    }
    return p;
}




static PyObject* g_pySerialModule = nullptr;
static PyObject* g_pyDocModule = nullptr;
static PyObject* g_pyArduinoObj = nullptr;
static PyObject* g_pyShorthandModule = nullptr;

std::wstring scriptDir = GetExeDir() + L"\\braille\\tools";
std::wstring sitePkgDir = GetExeDir() + L"\\Lib\\site-packages";
std::wstring sitePackagesDir = GetExeDir() + L"\\Lib\\site-packages";

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(n ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), n);
    return out;
}

static std::string WideToUtf8(const std::wstring& s) {
    if (s.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(n ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), n, nullptr, nullptr);
    return out;
}

static std::wstring PyErrorToString() {
    if (!PyErr_Occurred()) return L"Unknown Python error.";

    PyObject* ptype = nullptr, * pvalue = nullptr, * ptraceback = nullptr;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

    std::wstring msg = L"Python error";
    if (pvalue) {
        PyObject* strObj = PyObject_Str(pvalue);
        if (strObj) {
            const char* s = PyUnicode_AsUTF8(strObj);
            if (s) msg = Utf8ToWide(s);
            Py_DECREF(strObj);
        }
    }

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);
    return msg;
}

static bool InitEmbeddedPython(const std::wstring& scriptDir) {
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    PyRun_SimpleString("import sys");

    

    std::string dirUtf8 = WideToUtf8(scriptDir);
    std::string cmd = "sys.path.insert(0, r'" + dirUtf8 + "')";
    PyRun_SimpleString(cmd.c_str());

    if (!g_pySerialModule) {
        PyObject* name = PyUnicode_FromString("serial_braille");
        g_pySerialModule = PyImport_Import(name);
        Py_DECREF(name);
        if (!g_pySerialModule) return false;
    }

    if (!g_pyDocModule) {
        PyObject* name = PyUnicode_FromString("doc_to_braille");
        g_pyDocModule = PyImport_Import(name);
        Py_DECREF(name);
        if (!g_pyDocModule) return false;
    }

    if (!g_pyShorthandModule) {
        PyObject* name = PyUnicode_FromString("shorthand_braille");
        g_pyShorthandModule = PyImport_Import(name);
        Py_DECREF(name);
        if (!g_pyShorthandModule) return false;
    }

    return true;
}

static bool InitSerialPython(const std::wstring& scriptDir, const std::wstring& sitePackagesDir) {
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    PyRun_SimpleString("import sys");

    std::string s1 = WideToUtf8(scriptDir);
    std::string s2 = WideToUtf8(sitePackagesDir);

    std::string cmd =
        "import sys\n"
        "p1 = r'" + s1 + "'\n"
        "p2 = r'" + s2 + "'\n"
        "if p1 not in sys.path: sys.path.insert(0, p1)\n"
        "if p2 not in sys.path: sys.path.insert(0, p2)\n";

    PyRun_SimpleString(cmd.c_str());

    if (!g_pySerialModule) {
        PyObject* name = PyUnicode_FromString("serial_braille");
        g_pySerialModule = PyImport_Import(name);
        Py_DECREF(name);
        if (!g_pySerialModule) return false;
    }

    return true;
}

static bool InitDocPython(const std::wstring& scriptDir, const std::wstring& sitePackagesDir) {
    if (!Py_IsInitialized()) {
        Py_Initialize();
    }

    PyRun_SimpleString("import sys");

    std::string s1 = WideToUtf8(scriptDir);
    std::string s2 = WideToUtf8(sitePackagesDir);

    std::string cmd =
        "import sys\n"
        "p1 = r'" + s1 + "'\n"
        "p2 = r'" + s2 + "'\n"
        "if p1 not in sys.path: sys.path.insert(0, p1)\n"
        "if p2 not in sys.path: sys.path.insert(0, p2)\n";

    PyRun_SimpleString(cmd.c_str());

    if (!g_pyDocModule) {
        PyObject* name = PyUnicode_FromString("doc_to_braille");
        g_pyDocModule = PyImport_Import(name);
        Py_DECREF(name);
        if (!g_pyDocModule) return false;
    }

    return true;
}

static std::wstring PyFindArduinoPort() {
    if (!g_pySerialModule) return L"";
    PyObject* func = PyObject_GetAttrString(g_pySerialModule, "find_arduino_port");
    if (!func || !PyCallable_Check(func)) {
        Py_XDECREF(func);
        return L"";
    }

    PyObject* ret = PyObject_CallObject(func, nullptr);
    Py_DECREF(func);

    if (!ret) return L"";
    if (ret == Py_None) {
        Py_DECREF(ret);
        return L"";
    }

    const char* s = PyUnicode_AsUTF8(ret);
    std::wstring out = s ? Utf8ToWide(s) : L"";
    Py_DECREF(ret);
    return out;
}

static bool PyConnectArduino(const std::wstring& port, int baud = 115200) {
    if (!g_pySerialModule) return false;

    if (g_pyArduinoObj) {
        Py_DECREF(g_pyArduinoObj);
        g_pyArduinoObj = nullptr;
    }

    PyObject* cls = PyObject_GetAttrString(g_pySerialModule, "ArduinoBraille");
    if (!cls || !PyCallable_Check(cls)) {
        Py_XDECREF(cls);
        return false;
    }

    PyObject* args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, PyUnicode_FromString(WideToUtf8(port).c_str()));
    PyTuple_SetItem(args, 1, PyLong_FromLong(baud));

    g_pyArduinoObj = PyObject_CallObject(cls, args);

    Py_DECREF(args);
    Py_DECREF(cls);

    return g_pyArduinoObj != nullptr;
}

static void PyDisconnectArduino() {
    if (!g_pyArduinoObj) return;

    PyObject* ret = PyObject_CallMethod(g_pyArduinoObj, "close", nullptr);
    if (ret) Py_DECREF(ret);
    else PyErr_Clear();

    Py_DECREF(g_pyArduinoObj);
    g_pyArduinoObj = nullptr;
}

static bool PyPingArduino() {
    if (!g_pyArduinoObj) return false;
    PyObject* ret = PyObject_CallMethod(g_pyArduinoObj, "ping", nullptr);
    if (!ret) return false;
    bool ok = PyObject_IsTrue(ret);
    Py_DECREF(ret);
    return ok;
}

static bool PySendTextToArduino(const std::wstring& text, int delayMs = 600) {
    if (!g_pySerialModule || !g_pyArduinoObj) return false;

    PyObject* func = PyObject_GetAttrString(g_pySerialModule, "send_text");
    if (!func || !PyCallable_Check(func)) {
        Py_XDECREF(func);
        return false;
    }

    PyObject* args = PyTuple_New(3);
    Py_INCREF(g_pyArduinoObj);
    PyTuple_SetItem(args, 0, g_pyArduinoObj);
    PyTuple_SetItem(args, 1, PyUnicode_FromString(WideToUtf8(text).c_str()));
    PyTuple_SetItem(args, 2, PyLong_FromLong(delayMs));

    PyObject* ret = PyObject_CallObject(func, args);

    Py_DECREF(args);
    Py_DECREF(func);

    if (!ret) return false;
    Py_DECREF(ret);
    return true;
}

static bool PyExtractDocumentText(const std::wstring& filePath, std::wstring& outText) {
    if (!g_pyDocModule) return false;

    PyObject* extractFunc = PyObject_GetAttrString(g_pyDocModule, "extract_text");
    PyObject* cleanFunc = PyObject_GetAttrString(g_pyDocModule, "clean_extracted_text");
    if (!extractFunc || !cleanFunc || !PyCallable_Check(extractFunc) || !PyCallable_Check(cleanFunc)) {
        Py_XDECREF(extractFunc);
        Py_XDECREF(cleanFunc);
        return false;
    }

    PyObject* pathlib = PyImport_ImportModule("pathlib");
    if (!pathlib) {
        Py_DECREF(extractFunc);
        Py_DECREF(cleanFunc);
        return false;
    }

    PyObject* pathCls = PyObject_GetAttrString(pathlib, "Path");
    Py_DECREF(pathlib);
    if (!pathCls) {
        Py_DECREF(extractFunc);
        Py_DECREF(cleanFunc);
        return false;
    }

    PyObject* pathArg = PyTuple_New(1);
    PyTuple_SetItem(pathArg, 0, PyUnicode_FromString(WideToUtf8(filePath).c_str()));
    PyObject* pathObj = PyObject_CallObject(pathCls, pathArg);
    Py_DECREF(pathArg);
    Py_DECREF(pathCls);

    if (!pathObj) {
        Py_DECREF(extractFunc);
        Py_DECREF(cleanFunc);
        return false;
    }

    PyObject* extractArgs = PyTuple_New(1);
    PyTuple_SetItem(extractArgs, 0, pathObj); // steals ref
    PyObject* raw = PyObject_CallObject(extractFunc, extractArgs);
    Py_DECREF(extractArgs);
    Py_DECREF(extractFunc);

    if (!raw) {
        Py_DECREF(cleanFunc);
        return false;
    }

    PyObject* cleanArgs = PyTuple_New(1);
    PyTuple_SetItem(cleanArgs, 0, raw); // steals ref
    PyObject* cleaned = PyObject_CallObject(cleanFunc, cleanArgs);
    Py_DECREF(cleanArgs);
    Py_DECREF(cleanFunc);

    if (!cleaned) return false;

    const char* s = PyUnicode_AsUTF8(cleaned);
    outText = s ? Utf8ToWide(s) : L"";
    Py_DECREF(cleaned);
    return true;
}

static void ShutdownEmbeddedPython() {
    PyDisconnectArduino();

    Py_XDECREF(g_pySerialModule);
    g_pySerialModule = nullptr;

    Py_XDECREF(g_pyDocModule);
    g_pyDocModule = nullptr;

    Py_XDECREF(g_pyShorthandModule);
    g_pyShorthandModule = nullptr;

    if (Py_IsInitialized()) {
        Py_Finalize();
    }
}

static bool PyExtractPdfText(const std::wstring& filePath, std::wstring& outText) {
    PyObject* modName = PyUnicode_FromString("pdf_to_braille");
    PyObject* module = PyImport_Import(modName);
    Py_DECREF(modName);

    if (!module) return false;

    PyObject* extractFunc = PyObject_GetAttrString(module, "extract_text_from_pdf");
    PyObject* cleanFunc = PyObject_GetAttrString(module, "clean_extracted_text");

    if (!extractFunc || !cleanFunc ||
        !PyCallable_Check(extractFunc) || !PyCallable_Check(cleanFunc)) {
        Py_XDECREF(extractFunc);
        Py_XDECREF(cleanFunc);
        Py_DECREF(module);
        return false;
    }

    PyObject* args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, PyUnicode_FromString(WideToUtf8(filePath).c_str()));

    PyObject* raw = PyObject_CallObject(extractFunc, args);
    Py_DECREF(args);
    Py_DECREF(extractFunc);

    if (!raw) {
        Py_DECREF(cleanFunc);
        Py_DECREF(module);
        return false;
    }

    PyObject* cleanArgs = PyTuple_New(1);
    PyTuple_SetItem(cleanArgs, 0, raw); // steals ref
    PyObject* cleaned = PyObject_CallObject(cleanFunc, cleanArgs);
    Py_DECREF(cleanArgs);
    Py_DECREF(cleanFunc);
    Py_DECREF(module);

    if (!cleaned) return false;

    const char* s = PyUnicode_AsUTF8(cleaned);
    outText = s ? Utf8ToWide(s) : L"";
    Py_DECREF(cleaned);

    return true;
}

static bool IsTextUsable(const std::wstring& text) {
    size_t letters = 0;
    for (wchar_t ch : text) {
        if (!iswspace(ch)) {
            ++letters;
        }
    }
    return letters >= 10;
}

static bool PySendShorthandTextToArduino(const std::wstring& text, int delayMs = 600) {
    if (!g_pyShorthandModule || !g_pyArduinoObj) return false;

    PyObject* func = PyObject_GetAttrString(g_pyShorthandModule, "to_shorthand_patterns");
    if (!func || !PyCallable_Check(func)) {
        Py_XDECREF(func);
        return false;
    }

    std::string utf8 = WideToUtf8(text);

    PyObject* arg = PyTuple_New(1);
    PyTuple_SetItem(arg, 0, PyUnicode_FromString(utf8.c_str()));

    PyObject* patterns = PyObject_CallObject(func, arg);
    Py_DECREF(arg);
    Py_DECREF(func);

    if (!patterns || !PyList_Check(patterns)) {
        Py_XDECREF(patterns);
        return false;
    }

    PyObject* sendPatternMethod = PyObject_GetAttrString(g_pyArduinoObj, "send_pattern");
    PyObject* clearMethod = PyObject_GetAttrString(g_pyArduinoObj, "clear");

    if (!sendPatternMethod || !PyCallable_Check(sendPatternMethod) ||
        !clearMethod || !PyCallable_Check(clearMethod)) {
        Py_XDECREF(sendPatternMethod);
        Py_XDECREF(clearMethod);
        Py_DECREF(patterns);
        return false;
    }

    Py_ssize_t n = PyList_Size(patterns);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* item = PyList_GetItem(patterns, i); // borrowed ref

        if (item == Py_None) {
            PyObject* r = PyObject_CallObject(clearMethod, nullptr);
            if (r) Py_DECREF(r);
            else {
                Py_DECREF(sendPatternMethod);
                Py_DECREF(clearMethod);
                Py_DECREF(patterns);
                return false;
            }
            Sleep(delayMs);
            continue;
        }

        long pattern = PyLong_AsLong(item);
        if (PyErr_Occurred()) {
            Py_DECREF(sendPatternMethod);
            Py_DECREF(clearMethod);
            Py_DECREF(patterns);
            return false;
        }

        PyObject* args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, PyLong_FromLong(pattern));
        PyObject* ret = PyObject_CallObject(sendPatternMethod, args);
        Py_DECREF(args);

        if (!ret) {
            Py_DECREF(sendPatternMethod);
            Py_DECREF(clearMethod);
            Py_DECREF(patterns);
            return false;
        }

        bool ok = PyObject_IsTrue(ret);
        Py_DECREF(ret);

        if (!ok) {
            Py_DECREF(sendPatternMethod);
            Py_DECREF(clearMethod);
            Py_DECREF(patterns);
            return false;
        }

        Sleep(delayMs);
    }

    PyObject* r = PyObject_CallObject(clearMethod, nullptr);
    if (r) Py_DECREF(r);

    Py_DECREF(sendPatternMethod);
    Py_DECREF(clearMethod);
    Py_DECREF(patterns);
    return true;
}















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

static winrt::Windows::Foundation::IAsyncOperation<winrt::hstring>OcrPdfFileAsync(const std::wstring& path)
{
    using namespace winrt;
    using namespace winrt::Windows::Storage;
    using namespace winrt::Windows::Data::Pdf;
    using namespace winrt::Windows::Storage::Streams;
    using namespace winrt::Windows::Graphics::Imaging;

    std::wstring allText;

    StorageFile file = co_await StorageFile::GetFileFromPathAsync(path);
    PdfDocument pdf = co_await PdfDocument::LoadFromFileAsync(file);

    for (uint32_t i = 0; i < pdf.PageCount(); ++i) {
        PdfPage page = pdf.GetPage(i);

        InMemoryRandomAccessStream stream;
        co_await page.RenderToStreamAsync(stream);

        BitmapDecoder decoder = co_await BitmapDecoder::CreateAsync(stream);
        SoftwareBitmap sb = co_await decoder.GetSoftwareBitmapAsync();

        if (sb.BitmapPixelFormat() != BitmapPixelFormat::Bgra8 ||
            sb.BitmapAlphaMode() != BitmapAlphaMode::Premultiplied)
        {
            sb = SoftwareBitmap::Convert(
                sb,
                BitmapPixelFormat::Bgra8,
                BitmapAlphaMode::Premultiplied
            );
        }

        winrt::hstring pageText = co_await OcrBitmapAsync(sb);
        allText += pageText.c_str();

        if (i + 1 < pdf.PageCount()) {
            allText += L"\r\n\r\n";
        }
    }

    co_return winrt::hstring(allText);
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

/*
static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), out.data(), len, nullptr, nullptr);
    return out;
}
*/

static bool LoadTextFileToEdit(HWND hEdit, const std::wstring& path) {
    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) return false;

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string bytes = oss.str();

    // Handle UTF-8 BOM
    if (bytes.size() >= 3 && (uint8_t)bytes[0] == 0xEF && (uint8_t)bytes[1] == 0xBB && (uint8_t)bytes[2] == 0xBF) {
        bytes.erase(0, 3);
    }

    std::wstring w;
    if (!bytes.empty()) {
        int need = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
        if (need > 0) {
            w.resize(need);
            MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), w.data(), need);
        }
    }
    SetWindowTextW(hEdit, w.c_str());
    return true;
}

static bool SaveEditToTextFile(HWND hEdit, const std::wstring& path) {
    int len = GetWindowTextLengthW(hEdit);
    std::wstring w;
    w.resize(len);
    GetWindowTextW(hEdit, w.data(), len + 1);

    std::string utf8 = WideToUtf8(w);

    std::ofstream file(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!file) return false;

    // Optional: write UTF-8 BOM for Notepad compatibility
    const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
    file.write((const char*)bom, 3);

    file.write(utf8.data(), (std::streamsize)utf8.size());
    return true;
}

static bool PickOpenTxtFile(HWND owner, std::wstring& outPath) {
    wchar_t path[MAX_PATH] = L"";

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter =
        L"Supported Files (*.txt;*.docx;*.pdf)\0*.txt;*.docx;*.pdf\0"
        L"Text Files (*.txt)\0*.txt\0"
        L"Word Documents (*.docx)\0*.docx\0"
        L"PDF Files (*.pdf)\0*.pdf\0"
        L"All Files (*.*)\0*.*\0";
    //ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        outPath = path;
        return true;
    }
    return false;
}

static bool PickSaveTxtFile(HWND owner, std::wstring& outPath) {
    wchar_t path[MAX_PATH] = L"";

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameW(&ofn)) return false;

    outPath = path;
    // ensure .txt if user omitted extension
    if (outPath.find(L'.') == std::wstring::npos) outPath += L".txt";
    return true;
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
        }
        else {
            SetWindowTextW(hEditText, L"");
        }
    }
    catch (...) {
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

    dcb.BaudRate = CBR_115200;
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

static winrt::Windows::Graphics::Imaging::SoftwareBitmap
UpscaleIfLowResolution(
    winrt::Windows::Graphics::Imaging::SoftwareBitmap const& src,
    int minWidth = 1000,
    int scaleFactor = 2)
{
    using namespace winrt;
    using namespace winrt::Windows::Graphics::Imaging;

    if (!src) return nullptr;

    // Ensure format is BGRA8
    SoftwareBitmap input = src;
    if (input.BitmapPixelFormat() != BitmapPixelFormat::Bgra8)
    {
        input = SoftwareBitmap::Convert(
            input,
            BitmapPixelFormat::Bgra8,
            BitmapAlphaMode::Ignore
        );
    }

    int w = input.PixelWidth();
    int h = input.PixelHeight();

    // If resolution is already sufficient, return original
    if (w >= minWidth)
        return input;

    int newW = w * scaleFactor;
    int newH = h * scaleFactor;

    SoftwareBitmap output(
        BitmapPixelFormat::Bgra8,
        newW,
        newH,
        BitmapAlphaMode::Ignore
    );

    auto inBuf = input.LockBuffer(BitmapBufferAccessMode::Read);
    auto outBuf = output.LockBuffer(BitmapBufferAccessMode::Write);

    auto inDesc = inBuf.GetPlaneDescription(0);
    auto outDesc = outBuf.GetPlaneDescription(0);

    struct __declspec(uuid("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D"))
        IMemoryBufferByteAccess : IUnknown {
        virtual HRESULT __stdcall GetBuffer(uint8_t** buffer, uint32_t* capacity) = 0;
    };

    auto inRef = inBuf.CreateReference();
    auto outRef = outBuf.CreateReference();

    uint8_t* inPtr = nullptr;
    uint32_t inCap = 0;
    uint8_t* outPtr = nullptr;
    uint32_t outCap = 0;

    winrt::check_hresult(inRef.as<IMemoryBufferByteAccess>()->GetBuffer(&inPtr, &inCap));
    winrt::check_hresult(outRef.as<IMemoryBufferByteAccess>()->GetBuffer(&outPtr, &outCap));

    const int bytesPerPixel = 4;

    // Bilinear interpolation
    for (int y = 0; y < newH; ++y)
    {
        float srcY = (float)y / scaleFactor;
        int y0 = (int)srcY;
        int y1 = min(y0 + 1, h - 1);
        float wy = srcY - y0;

        for (int x = 0; x < newW; ++x)
        {
            float srcX = (float)x / scaleFactor;
            int x0 = (int)srcX;
            int x1 = min(x0 + 1, w - 1);
            float wx = srcX - x0;

            uint8_t* p00 = inPtr + y0 * inDesc.Stride + x0 * bytesPerPixel;
            uint8_t* p10 = inPtr + y0 * inDesc.Stride + x1 * bytesPerPixel;
            uint8_t* p01 = inPtr + y1 * inDesc.Stride + x0 * bytesPerPixel;
            uint8_t* p11 = inPtr + y1 * inDesc.Stride + x1 * bytesPerPixel;

            uint8_t* dst = outPtr + y * outDesc.Stride + x * bytesPerPixel;

            for (int c = 0; c < 4; ++c)
            {
                float val =
                    (1 - wx) * (1 - wy) * p00[c] +
                    wx * (1 - wy) * p10[c] +
                    (1 - wx) * wy * p01[c] +
                    wx * wy * p11[c];

                dst[c] = (uint8_t)val;
            }
        }
    }

    return output;
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


LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            switch (wParam)
            {
            case 'A':
                SendMessageW(hwnd, EM_SETSEL, 0, -1);
                return 0;

            case 'S':
            {
                std::wstring path;
                if (PickSaveTxtFile(hWndMain, path))
                {
                    SaveEditToTextFile(hwnd, path);
                }
                return 0;
            }

            case 'O':
            {
                std::wstring path;
                if (PickOpenTxtFile(hWndMain, path))
                {
                    LoadTextFileToEdit(hwnd, path);
                }
                return 0;
            }
            }
        }
    }

    return CallWindowProcW(OriginalEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        int margin = 10, marginw = 140, rowH = 40, btnW = 100;
        hCbPorts = CreateWindowW(L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, marginw, margin+5, 180, 200, hWnd, (HMENU)IDC_COMBO_PORTS, hInst, nullptr);
        hBtnRefresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE, marginw + 190, margin, btnW, rowH, hWnd, (HMENU)IDC_BTN_REFRESH, hInst, nullptr);
        hBtnConnect = CreateWindowW(L"BUTTON", L"Connect", WS_CHILD | WS_VISIBLE, marginw + 300, margin, btnW, rowH, hWnd, (HMENU)IDC_BTN_CONNECT, hInst, nullptr);
        hEditText = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL, marginw, margin + rowH + 10, 500, 240, hWnd, (HMENU)IDC_EDIT_TEXT, hInst, nullptr);
        OriginalEditProc = (WNDPROC)SetWindowLongPtrW(hEditText, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

        hBtnOpen = CreateWindowW(L"BUTTON", L"Open", WS_CHILD | WS_VISIBLE, marginw, margin + rowH + 260, btnW, rowH, hWnd, (HMENU)IDC_BTN_OPEN, hInst, nullptr);
        hBtnSave = CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE, marginw + 110, margin + rowH + 260, btnW, rowH, hWnd, (HMENU)IDC_BTN_SAVE, hInst, nullptr);
        hBtnSend = CreateWindowW(L"BUTTON", L"Send", WS_CHILD | WS_VISIBLE, marginw + 220, margin + rowH + 260, btnW, rowH, hWnd, (HMENU)IDC_BTN_SEND, hInst, nullptr);

        hRadioRegular = CreateWindowW(L"BUTTON", L"Regular", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, margin, margin + rowH + 10, 120, rowH, hWnd, (HMENU)IDC_RADIO_REGULAR, hInst, nullptr);
        hRadioShorthand = CreateWindowW(L"BUTTON", L"Shorthand", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, margin, margin + (rowH + 10)*2, 120, rowH, hWnd, (HMENU)IDC_RADIO_SHORTHAND, hInst, nullptr);
        hBtnRegion = CreateWindowW(L"BUTTON", L"OCR (region)", WS_CHILD | WS_VISIBLE, margin, margin + (rowH + 10) * 3, 120, rowH, hWnd, (HMENU)IDC_BTN_OCR_REGION, hInst, nullptr);
        hBtnFull = CreateWindowW(L"BUTTON", L"OCR (full)", WS_CHILD | WS_VISIBLE, margin, margin + (rowH + 10) * 4, 120, rowH, hWnd, (HMENU)IDC_BTN_OCR_FULLSCREEN, hInst, nullptr);

        SendMessageW(hRadioRegular, BM_SETCHECK, BST_CHECKED, 0);
        g_useShorthand = false;

        PopulatePorts();
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_REFRESH && HIWORD(wParam) == BN_CLICKED) {
            /*
            if (!InitEmbeddedPython(scriptDir)) {
                MsgBox((L"Python init/import failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                return 0;
            }
            */
            
            if (!InitSerialPython(scriptDir, sitePackagesDir)) {
                MsgBox((L"Python serial init/import failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                return 0;
            }
            

            std::wstring port = PyFindArduinoPort();
            if (port.empty()) {
                MsgBox(L"No Arduino port found by Python.", MB_ICONWARNING);
            }
            else {
                int idx = (int)SendMessageW(hCbPorts, CB_FINDSTRINGEXACT, -1, (LPARAM)port.c_str());
                if (idx == CB_ERR) {
                    SendMessageW(hCbPorts, CB_ADDSTRING, 0, (LPARAM)port.c_str());
                    idx = (int)SendMessageW(hCbPorts, CB_FINDSTRINGEXACT, -1, (LPARAM)port.c_str());
                }
                if (idx != CB_ERR) SendMessageW(hCbPorts, CB_SETCURSEL, idx, 0);
                MsgBox((L"Python detected port: " + port).c_str());
            }
        }
        else if (id == IDC_BTN_CONNECT && HIWORD(wParam) == BN_CLICKED) {
            /*
            if (!InitEmbeddedPython(scriptDir)) {
                MsgBox((L"Python init/import failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                return 0;
            }
            */
            
            if (!InitSerialPython(scriptDir, sitePackagesDir)) {
                MsgBox((L"Python serial init/import failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                return 0;
            }
            

            if (g_pyArduinoObj) {
                PyDisconnectArduino();
                MsgBox(L"Disconnected.");
            }
            else {
                std::wstring sel = GetSelectedPort();
                if (sel.empty()) {
                    sel = PyFindArduinoPort();
                }
                if (sel.empty()) {
                    MsgBox(L"No port selected / detected.", MB_ICONWARNING);
                    return 0;
                }

                if (!PyConnectArduino(sel, 115200)) {
                    MsgBox((L"Python connect failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                    return 0;
                }

                if (!PyPingArduino()) {
                    MsgBox((L"Connected to " + sel + L", but ping failed.").c_str(), MB_ICONWARNING);
                }
                else {
                    MsgBox((L"Connected to " + sel).c_str());
                }
            }
        }
        else if (id == IDC_BTN_SEND && HIWORD(wParam) == BN_CLICKED) {
            wchar_t buf[2048];
            GetWindowTextW(hEditText, buf, 2048);
            if (wcslen(buf) == 0) {
                MsgBox(L"Nothing to send.", MB_ICONWARNING);
            }
            else if (!g_pyArduinoObj) {
                MsgBox(L"Not connected.", MB_ICONWARNING);
            }
            else {
                bool ok = false;

                if (g_useShorthand) {
                    ok = PySendShorthandTextToArduino(buf, 600);
                }
                else {
                    ok = PySendTextToArduino(buf, 600);
                }

                if (!ok) {
                    MsgBox((L"Python send failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                }
                else {
                    MsgBox(g_useShorthand ? L"Sent successfully (shorthand)." : L"Sent successfully (regular).");
                }
            }
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

                    cropped = UpscaleIfLowResolution(cropped, 800, 2);
                    cropped = UpscaleIfLowResolution(cropped, 200, 2);

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
        else if (id == IDC_BTN_OPEN && HIWORD(wParam) == BN_CLICKED) {
            std::wstring path;
            if (PickOpenTxtFile(hWndMain, path)) {
                /*
                if (!InitEmbeddedPython(scriptDir)) {
                    MsgBox((L"Python init/import failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                    return 0;
                }
                */
                
                if (!InitDocPython(scriptDir, sitePackagesDir)) {
                    MsgBox((L"Python doc init/import failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                    return 0;
                }
                

                std::wstring ext = std::filesystem::path(path).extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

                if (ext == L".pdf") {
                    std::wstring text;
                    bool ok = PyExtractPdfText(path, text);

                    if (ok && IsTextUsable(text)) {
                        SetWindowTextW(hEditText, text.c_str());
                    }
                    else {
                        PostToEdit(L"[PDF] No usable embedded text found. Running OCR...");

                        winrt::fire_and_forget([path]() -> winrt::fire_and_forget {
                            try {
                                winrt::hstring textH = co_await OcrPdfFileAsync(path);
                                std::wstring t = textH.c_str();

                                if (!IsTextUsable(t)) {
                                    PostToEdit(L"[PDF OCR] No readable text found.");
                                }
                                else {
                                    auto* p = new std::wstring(t);
                                    PostMessageW(hWndMain, WM_APP + 2, 0, (LPARAM)p);
                                }
                            }
                            catch (winrt::hresult_error const& e) {
                                std::wstring msg =
                                    L"[PDF OCR] hresult_error 0x" + std::to_wstring((uint32_t)e.code().value) +
                                    L"\n" + HrToStr(e.code()) +
                                    L"\n" + std::wstring(e.message().c_str());
                                PostToEdit(msg);
                            }
                            catch (...) {
                                PostToEdit(L"[PDF OCR] Failed.");
                            }
                            co_return;
                            }());
                    }
                }
                else {
                    std::wstring text;
                    if (!PyExtractDocumentText(path, text)) {
                        MsgBox((L"Document extract failed:\n" + PyErrorToString()).c_str(), MB_ICONERROR);
                    }
                    else {
                        SetWindowTextW(hEditText, text.c_str());
                    }
                }
            }
            }
        else if (id == IDC_BTN_SAVE && HIWORD(wParam) == BN_CLICKED) {
            std::wstring path;
            if (PickSaveTxtFile(hWndMain, path)) {
                if (!SaveEditToTextFile(hEditText, path)) MsgBox(L"Save failed.", MB_ICONERROR);
            }
        }
        else if (id == IDC_RADIO_REGULAR && HIWORD(wParam) == BN_CLICKED) {
            g_useShorthand = false;
        }
        else if (id == IDC_RADIO_SHORTHAND && HIWORD(wParam) == BN_CLICKED) {
            g_useShorthand = true;
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
    case WM_APP + 2: {
        auto* p = reinterpret_cast<std::wstring*>(lParam);
        if (p) {
            SetWindowTextW(hEditText, p->c_str());
            delete p;
        }
        return 0;
    }
    case WM_DESTROY: ShutdownEmbeddedPython(); CloseSerial(); PostQuitMessage(0); return 0;
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
    wc.lpszClassName = L"BrailleDriver";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    hWndMain = CreateWindowW(L"BrailleDriver", L"Braille Driver v1.4.0", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
        670, 410,
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


