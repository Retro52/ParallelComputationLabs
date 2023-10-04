#pragma once

#include <core/include/Layer.hpp>
#include <core/include/Timer.hpp>
#include <core/include/Window.hpp>

#include <memory>
#include <wrappers/include/winmutex.hpp>
#include <wrappers/include/winthread.hpp>

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

        core::Timer m_all_threads_run_timer;

        std::vector<thread::winthread> m_threads;

        std::unique_ptr<graphics::Window> m_window;

    };
}