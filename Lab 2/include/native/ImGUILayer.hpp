#pragma once

#include <core/Layer.hpp>
#include <core/Timer.hpp>
#include <graphics/Window.hpp>

#include <memory>
#include <wrapper/winmutex.hpp>
#include <wrapper/winthread.hpp>

namespace retro
{
    class ImGUILayer
        : public core::Layer
    {
    public:

        void OnAttach() override;

        void OnDetach() override;

        bool OnUpdate(ts delta) override;

        bool OnEvent(const core::Event &event) override;

    private:

        void Render();

    protected:

        std::vector<thread::winthread> m_threads;

        std::unique_ptr<graphics::Window> m_window;

    };
}