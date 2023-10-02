#include <core/Event.hpp>
#include <wrapper/winmutex.hpp>

using namespace retro::core;

static retro::mutex::winmutex poll_mutex;

void EventsPoll::ClearEvents()
{
    m_events.clear();
}

void EventsPoll::AddEvent(const Event &event)
{
    poll_mutex.lock();

    m_events.push_back(event);

    poll_mutex.unlock();
}

const std::vector<Event> &EventsPoll::GetEvents()
{
    return m_events;
}
