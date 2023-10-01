#pragma once

#include <vector>
#include <SDL.h>

namespace retro::core
{
    using Event = SDL_Event;

    class EventsPoll
    {
    public:

        static void ClearEvents();

        static void AddEvent(const Event& event);

        static const std::vector<Event>& GetEvents();

    protected:

        inline static std::vector<Event> m_events;

    };
}