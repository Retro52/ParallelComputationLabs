#include <wrapper/winthread.hpp>

#include <stdexcept>

using namespace retro::thread;

namespace
{
    auto get_native_priority(priority e_priority)
    {
        switch (e_priority)
        {
            case priority::low:
                return THREAD_PRIORITY_LOWEST;

            case priority::below_normal:
                return THREAD_PRIORITY_BELOW_NORMAL;

            case priority::normal:
                return THREAD_PRIORITY_NORMAL;

            case priority::above_normal:
                return THREAD_PRIORITY_ABOVE_NORMAL;

            case priority::high:
                return THREAD_PRIORITY_HIGHEST;
        }
    }
}

winthread::~winthread()
{
    if (m_is_running && !m_is_finished)
    {
        join();
    }

    if (m_hThread != nullptr)
    {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
    }
}

void winthread::run()
{
    if (m_invoke)
    {
        if (m_is_running && !m_is_finished)
        {
            join();
        }

        m_hThread = CreateThread(nullptr, 0, thread_function, this, 0, nullptr   );

        if (m_hThread == nullptr)
        {
            throw std::runtime_error("Error: the thread could not be created");
        }

        m_is_running = true;
        m_is_finished = false;
    }
    else
    {
        throw std::runtime_error("Error: Runnable not assigned");
    }
}

void winthread::join()
{
    if (m_hThread == nullptr)
    {
        throw std::runtime_error("Error: No thread to join");
    }

    if (m_is_running && !m_is_paused)
    {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);

        m_hThread = nullptr;

        m_is_running = false;
        m_is_finished = true;
    }
}

DWORD WINAPI winthread::thread_function(LPVOID lpParam)
{
    auto * pThis = static_cast<winthread *>(lpParam);
    pThis->m_invoke();

    pThis->m_is_running = false;
    pThis->m_is_finished = true;
    return 0;
}

void winthread::pause()
{
    if (m_is_running)
    {
        m_is_paused = true;
        SuspendThread(m_hThread);
    }
    else
    {
        throw std::runtime_error("Error: No running thread to pause");
    }
}

void winthread::resume()
{
    if (m_is_running)
    {
        m_is_paused = false;
        ResumeThread(m_hThread);
    }
    else
    {
        throw std::runtime_error("Error: No running thread to resume");
    }
}

void winthread::terminate()
{
    if (m_is_running && !m_is_finished)
    {
        TerminateThread(m_hThread, 0);
        CloseHandle(m_hThread);

        m_hThread = nullptr;

        m_is_paused = false;
        m_is_running = false;

        m_is_finished = true;
    }
    else
    {
        throw std::runtime_error("Error: No running thread to stop");
    }
}

priority winthread::get_priority()
{
    return m_priority;
}

void winthread::set_priority(priority priority)
{
    m_priority = priority;

    if (!m_is_running || !m_hThread)
    {
        return;
    }

    if (!SetThreadPriority(m_hThread, get_native_priority(priority)))
    {
        throw std::runtime_error("Error: Failed to set thread priority");
    }
}

[[nodiscard]] bool winthread::is_paused() const
{
    return m_is_paused;
}

[[nodiscard]] bool winthread::is_running() const
{
    return m_is_running;
}

[[nodiscard]] bool winthread::is_finished() const
{
    return m_is_finished;
}
