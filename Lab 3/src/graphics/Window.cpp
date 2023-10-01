#include <core/Event.hpp>
#include <graphics/Window.hpp>

#include <tchar.h>

#include <stdexcept>
#include <SDL_syswm.h>

#include <backends/imgui_impl_sdl2.h>

using namespace retro::graphics;

Window::Window()
    : Window("Lab 1 by Retro52", 1280, 720)
{
    
}

Window::Window(const std::string &name, int32_t width, int32_t height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        const std::string reason = SDL_GetError();
        throw std::runtime_error("Failed to initialize SDL. Reason: " + reason);
    }

    SDL_SysWMinfo wmInfo;
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);

    SDL_VERSION(&wmInfo.version)
    SDL_GetWindowWMInfo(m_window, &wmInfo);
    HWND hwnd = (HWND)wmInfo.info.win.window;

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        throw std::runtime_error("Failed to initialize DirectX 11");
    }
}

Window::~Window()
{
    Close();
}

void Window::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        core::EventsPoll::AddEvent(event);

        if (event.type == SDL_WINDOWEVENT
            && event.window.event == SDL_WINDOWEVENT_RESIZED
            && event.window.windowID == SDL_GetWindowID(m_window))
        {
            CleanupRenderTarget();
            m_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
    }
}

void Window::Close()
{
    CleanupDeviceD3D();
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

SDL_Window * Window::GetWindowHandler() const
{
    return m_window;
}

ID3D11Device * Window::GetDirectXDevice() const
{
    return m_pd3dDevice;
}

IDXGISwapChain * Window::GetDirectXSwapChain() const
{
    return m_pSwapChain;
}

ID3D11DeviceContext * Window::GetDirectXDeviceContext() const
{
    return m_pd3dDeviceContext;
}


ID3D11RenderTargetView * Window::GetDirectXRenderTargetView() const
{
    return m_mainRenderTargetView;
}

// Helper functions
bool Window::CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
    {
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    }
    if (res != S_OK)
    {
        return false;
    }

    CreateRenderTarget();
    return true;
}

void Window::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pd3dDeviceContext) { m_pd3dDeviceContext->Release(); m_pd3dDeviceContext = nullptr; }
    if (m_pd3dDevice) { m_pd3dDevice->Release(); m_pd3dDevice = nullptr; }
}

void Window::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_mainRenderTargetView);
    pBackBuffer->Release();
}

void Window::CleanupRenderTarget()
{
    if (m_mainRenderTargetView) { m_mainRenderTargetView->Release(); m_mainRenderTargetView = nullptr; }
}
