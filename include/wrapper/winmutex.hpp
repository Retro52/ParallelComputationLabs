#pragma once

#if !defined(_WIN32) && !defined(WIN32)
# error "Incompatible platform"
#endif

#include <windows.h>
#include <stdexcept>

namespace retro::mutex
{
    class winmutex
    {
    public:

        winmutex();

        ~winmutex();

        void lock();

        void unlock();

    protected:

        HANDLE m_hMutex;

    };

    template<typename Mutex>
    class lock_guard
    {
    public:

        explicit lock_guard(Mutex mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
        }

        ~lock_guard()
        {
            m_mutex.unlock();
        }

    protected:

        Mutex m_mutex;
    };
}
