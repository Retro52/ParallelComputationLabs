#pragma once

#include <string>
#include <vector>

#include <SDL.h>
#include <d3d11.h>

namespace retro::graphics
{
    class Window
    {
    public:

        explicit Window();

        explicit Window(const std::string& name, int32_t width, int32_t height);

        ~Window();

        [[nodiscard]] std::vector<SDL_Event> PollEvents();

        void Close();

        [[nodiscard]] SDL_Window * GetWindowHandler() const;

        [[nodiscard]] ID3D11Device * GetDirectXDevice() const;

        [[nodiscard]] IDXGISwapChain * GetDirectXSwapChain() const;

        [[nodiscard]] ID3D11DeviceContext * GetDirectXDeviceContext() const;

        [[nodiscard]] ID3D11RenderTargetView * GetDirectXRenderTargetView() const;

    protected:

        void CleanupDeviceD3D();

        void CreateRenderTarget();

        void CleanupRenderTarget();

        bool CreateDeviceD3D(HWND hWnd);

    protected:

        SDL_Window * m_window;

        ID3D11Device * m_pd3dDevice { nullptr };
        IDXGISwapChain * m_pSwapChain { nullptr };
        ID3D11DeviceContext * m_pd3dDeviceContext { nullptr };
        ID3D11RenderTargetView *  m_mainRenderTargetView { nullptr };

    };
}