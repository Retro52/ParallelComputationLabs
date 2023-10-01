#include <wrapper/winmutex.hpp>

using namespace retro::mutex;

winmutex::winmutex()
{
    m_hMutex = CreateMutex(nullptr, FALSE, nullptr);
    
    if (m_hMutex == nullptr)
    {
        throw std::runtime_error("Failed to create mutex");
    }
}

winmutex::~winmutex()
{
    CloseHandle(m_hMutex);
}

void winmutex::lock()
{
    const auto result = WaitForSingleObject(m_hMutex, INFINITE);

    if (result != WAIT_OBJECT_0)
    {
        switch (result)
        {
            case WAIT_FAILED:
                throw std::runtime_error("Mutex lock failed because it failed");
            case WAIT_ABANDONED:
                throw std::runtime_error("Mutex abandoned, check for deadlocks");
            case WAIT_IO_COMPLETION:
                throw std::runtime_error("Mutex wait failed, I/O competition");
            default:
                throw std::runtime_error("Mutex wait for unknown reasons");
        }
    }
}

void winmutex::unlock()
{
    if (!ReleaseMutex(m_hMutex))
    {
        throw std::runtime_error("Failed to unlock mutex");
    }
}
