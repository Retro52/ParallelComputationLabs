#pragma once

#include <core/Layer.hpp>

#include <memory>
#include <unordered_map>

namespace retro::core
{
    class Application
    {
    public:

        ~Application();

        int Run();

        void RemoveLayer(const std::string& key);

        template<typename L, typename... Args>
        void EmplaceLayer(const std::string& key, Args&&... args)
        {
            if (!m_layers.contains(key))
            {
                m_layers[key] = std::make_unique<L>(std::forward<Args>(args)...);
                m_layers[key]->OnAttach();
            }
        }

    protected:

        bool m_is_running { true };

        std::unordered_map<std::string, std::unique_ptr<Layer>> m_layers;

    };
}