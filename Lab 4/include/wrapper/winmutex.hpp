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
}
