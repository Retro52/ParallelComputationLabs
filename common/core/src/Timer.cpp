#include <core/Timer.hpp>

#include <sstream>
#include <utility>
#include <iostream>

using namespace retro::core;

void Timer::Run()
{
    m_is_started = true;
    m_is_finished = false;
    start = std::chrono::high_resolution_clock::now();
}

void Timer::Stop()
{
    m_is_started = false;
    m_is_finished = true;
    end = std::chrono::high_resolution_clock::now();
}

ScopeTimer::ScopeTimer()
    : ScopeTimer(GetNextUniqueID())
{
}

ScopeTimer::ScopeTimer(std::string name)
    : m_id(std::move(name))
{
    Run();
    ++m_active_timers;
}

ScopeTimer::~ScopeTimer()
{
    Stop();
    auto elapsed = Tick<std::chrono::microseconds>();

    std::cout << "Timer '" << m_id << "' Elapsed in: '" << elapsed.count() << "' microseconds" << std::endl;

    --m_active_timers;
}

std::string ScopeTimer::GetNextUniqueID()
{
    std::stringstream id;

    id << "Timer #";
    id << m_active_timers;

    return id.str();
}
