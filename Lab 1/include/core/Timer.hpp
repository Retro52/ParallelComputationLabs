#pragma once

#include <chrono>
#include <string>

namespace retro::core
{
    class Timer
    {
    public:

        void Run();

        void Stop();

        template<typename T>
        T Tick() const
        {
            if (!m_is_started && !m_is_finished)
            {
                return T(0);
            }
            else if (!m_is_finished)
            {
                auto elapsed = std::chrono::high_resolution_clock::now() - start;
                return std::chrono::duration_cast<T>(elapsed);
            }

            return std::chrono::duration_cast<T>((end - start));
        }

    protected:

        bool m_is_started { false };

        bool m_is_finished { false };

        std::chrono::time_point<std::chrono::high_resolution_clock> end;

        std::chrono::time_point<std::chrono::high_resolution_clock> start;
    };


    class ScopeTimer
        : public Timer
    {
    public:

        explicit ScopeTimer();

        explicit ScopeTimer(std::string name);

        ~ScopeTimer();

    private:

        static std::string GetNextUniqueID();

    protected:

        std::string m_id;

    private:

        inline static int64_t m_active_timers;

    };
}