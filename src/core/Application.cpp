#include <core/Application.hpp>

#include <chrono>

using namespace retro::core;

Application::~Application()
{
    for (const auto& [key, layer] : m_layers)
    {
        layer->OnDetach();
    }
}

int Application::Run()
{
    Layer::ts duration = 0.0F;

    while (m_is_running)
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& [key, layer] : m_layers)
        {
            m_is_running &= layer->OnUpdate(duration);
        }

        const auto& events = core::EventsPoll::GetEvents();

        for (const auto& [key, layer] : m_layers)
        {
            for (const auto& event : events)
            {
                m_is_running &= layer->OnEvent(event);
            }
        }

        core::EventsPoll::ClearEvents();
        auto end = std::chrono::high_resolution_clock::now();
        duration = static_cast<decltype(duration)>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1000.0F;
    }

    return EXIT_SUCCESS;
}

void Application::RemoveLayer(const std::string& key)
{
    const auto layer = m_layers.find(key);

    if (layer != m_layers.end())
    {
        layer->second->OnDetach();
        m_layers.erase(layer);
    }
}