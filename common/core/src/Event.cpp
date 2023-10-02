#include <Event.hpp>

using namespace retro::core;

void EventsPoll::ClearEvents()
{
    m_events.clear();
}

void EventsPoll::AddEvent(const Event &event)
{
    m_events.push_back(event);
}

const std::vector<Event> &EventsPoll::GetEvents()
{
    return m_events;
}
