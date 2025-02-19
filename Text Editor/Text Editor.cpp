#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <wincodec.h>
#include <dwrite.h>
#include <string>
#pragma comment(lib, "Dwrite")
#pragma comment(lib, "d2d1.lib")
#include <new>



template<class Interface>
inline void SafeRelease(
	Interface** ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif




class TextEditor
{
public:
	TextEditor();
	~TextEditor();

	HRESULT Initialize();

	void RunMessageLoop();

private:
    HWND m_hwnd;
    ID2D1Factory* m_pDirect2dFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
    IDWriteFactory* m_pIDwriteFactory;
    IDWriteTextFormat* m_pITextFormat;
    std::wstring userInput;

	HRESULT CreateDeviceIndepentResources();
	
	HRESULT CreateDeviceResources();

	void DiscardDeviceResources();

	HRESULT onRender();

	void OnResize(
		UINT width,
		UINT height
	);

	static LRESULT CALLBACK WndProc(
		HWND hwnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);
};

TextEditor::TextEditor() :
    m_hwnd(NULL),
    m_pDirect2dFactory(NULL),
    m_pRenderTarget(NULL),
    m_pLightSlateGrayBrush(NULL),
    m_pITextFormat(NULL),
    m_pIDwriteFactory(NULL)

{}

TextEditor::~TextEditor()
{
    SafeRelease(& m_pDirect2dFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pLightSlateGrayBrush);
    SafeRelease(&m_pITextFormat);
    SafeRelease(&m_pIDwriteFactory);
}

void TextEditor::RunMessageLoop()
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}



void TextEditor::OnResize(UINT width, UINT height)
{
    if (m_pRenderTarget)
    {
        // Note: This method can fail, but it's okay to ignore the
        // error here, because the error will be returned again
        // the next time EndDraw is called.
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}

HRESULT TextEditor::CreateDeviceIndepentResources()
{
    HRESULT hr = S_OK;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
    return hr;
}

HRESULT TextEditor::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    if (!m_pRenderTarget)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        // Create a Direct2D render target.
        hr = m_pDirect2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRenderTarget
        );

        if (SUCCEEDED(hr))
        {
            hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(&m_pIDwriteFactory));
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(0.83f, 0.83f, 0.83f, 1.0f),
                &m_pLightSlateGrayBrush);
        }

        if (SUCCEEDED(hr))
        {
            hr = m_pIDwriteFactory->CreateTextFormat(L"Arial",
                NULL, DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                10.0f * 96.0f / 72.0f,
                L"en-US",
                &m_pITextFormat);
        }

    }

    return hr;
}



void TextEditor::DiscardDeviceResources()
{
    SafeRelease(&m_pDirect2dFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pLightSlateGrayBrush);
    SafeRelease(&m_pITextFormat);
    SafeRelease(&m_pIDwriteFactory);
}

HRESULT TextEditor::Initialize()
{
    HRESULT hr;

    // Initialize device-independent resources, such
    // as the Direct2D factory.
    hr = CreateDeviceIndepentResources();

    if (SUCCEEDED(hr))
    {
        // Register the window class.
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = TextEditor::WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = HINST_THISCOMPONENT;
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName = NULL;
        wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
        wcex.lpszClassName = L"D2DDemoApp";

        RegisterClassEx(&wcex);

        // In terms of using the correct DPI, to create a window at a specific size
        // like this, the procedure is to first create the window hidden. Then we get
        // the actual DPI from the HWND (which will be assigned by whichever monitor
        // the window is created on). Then we use SetWindowPos to resize it to the
        // correct DPI-scaled size, then we use ShowWindow to show it.

        m_hwnd = CreateWindow(
            L"D2DDemoApp",
            L"Text Editor",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this);

        if (m_hwnd)
        {
            // Because the SetWindowPos function takes its size in pixels, we
            // obtain the window's DPI, and use it to scale the window size.
            float dpi = GetDpiForWindow(m_hwnd);

            SetWindowPos(
                m_hwnd,
                NULL,
                NULL,
                NULL,
                static_cast<int>(ceil(640.f * dpi / 96.f)),
                static_cast<int>(ceil(480.f * dpi / 96.f)),
                SWP_NOMOVE);
            ShowWindow(m_hwnd, SW_SHOWNORMAL);
            UpdateWindow(m_hwnd);

        }

        
    }

    return hr;

}

LRESULT CALLBACK TextEditor::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        TextEditor* pDemoApp = (TextEditor*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pDemoApp)
        );

        result = 1;
    }
    else
    {
        TextEditor* pDemoApp = reinterpret_cast<TextEditor*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        wchar_t msg[32];

        switch (message)
        {
        case WM_SYSKEYDOWN:
            swprintf_s(msg, L"WM_SYSKEYDOWN: 0x%x\n", wParam);
            OutputDebugString(msg);
            break;
        case WM_SYSCHAR:
            swprintf_s(msg, L"WM_SYSCHAR: %c\n", (wchar_t)wParam);
            OutputDebugString(msg);
        case WM_SYSKEYUP:
            swprintf_s(msg, L"WM_SYSKEYUP: 0x%x\n", wParam);
            OutputDebugString(msg);
        case WM_CHAR:
            if(wParam == VK_BACK){
                if(!pDemoApp->userInput.empty())
                pDemoApp->userInput.pop_back();
                //InvalidateRect(hwnd, 0, TRUE);
            }
            else {
                swprintf_s(msg, L"WM_CHAR: %c\n", (wchar_t)wParam);
                OutputDebugString(msg);
                pDemoApp->userInput.push_back((wchar_t)wParam);
            }
            
        default:
            break;
        }

        if (pDemoApp)
        {
            switch (message)
            {
            case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                pDemoApp->OnResize(width, height);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DISPLAYCHANGE:
            {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_PAINT:
            {
                pDemoApp->onRender();
                //ValidateRect(hwnd, NULL);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DESTROY:
            {
                PostQuitMessage(0);
            }
            result = 1;
            wasHandled = true;
            break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
        
    }
    return result;

}

HRESULT TextEditor::onRender()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    D2D1_RECT_F layoutRect = D2D1::RectF(
        static_cast<float>(rc.left),
        static_cast<float>(rc.top),
        static_cast<float>(rc.right),
        static_cast<float>(rc.bottom)
    );


    hr = CreateDeviceResources();
    if (SUCCEEDED(hr))
    {
        m_pRenderTarget->BeginDraw();
        m_pRenderTarget->Clear(D2D1::ColorF(0x1E1E1E));
        m_pRenderTarget->DrawText(
            userInput.c_str(),
            static_cast<UINT32>(userInput.length()),
            m_pITextFormat,
            layoutRect,
            m_pLightSlateGrayBrush
        );
    }

    if (hr == D2DERR_RECREATE_TARGET)
    {
        hr = S_OK;
        DiscardDeviceResources();
    }

    hr = m_pRenderTarget->EndDraw();


    return hr;

}

int WINAPI WinMain(
    HINSTANCE /* hInstance */,
    HINSTANCE /* hPrevInstance */,
    LPSTR /* lpCmdLine */,
    int /* nCmdShow */
)
{
    // Use HeapSetInformation to specify that the process should
    // terminate if the heap manager detects an error in any heap used
    // by the process.
    // The return value is ignored, because we want to continue running in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            TextEditor app;

            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}



	
