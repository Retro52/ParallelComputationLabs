#pragma once

#include <string>
#include "Event.hpp"

namespace retro::core
{
    class Layer
    {
    public:

        using ts = float;

        virtual void OnAttach() = 0;

        virtual void OnDetach() = 0;

        virtual bool OnUpdate(ts delta) = 0;

        virtual bool OnEvent(const Event& event) = 0;

    };
}