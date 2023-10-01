#pragma once

#if !defined(_WIN32) && !defined(WIN32)
# error "Incompatible platform"
#endif

#include <windows.h>
#include <functional>

namespace retro::thread
{
    enum class priority
    {
        low,
        below_normal,
        normal,
        above_normal,
        high
    };

    class winthread
    {
    public:
        explicit winthread() = default;

        ~winthread();

        void run();

        void join();

        void pause();

        void resume();

        void terminate();

        priority get_priority();

        void set_priority(priority priority);

        [[nodiscard]] bool is_paused() const;

        [[nodiscard]] bool is_running() const;

        [[nodiscard]] bool is_finished() const;

        template<
                typename Func,
                typename... Args,
                typename = std::enable_if_t<std::is_invocable_v<Func, Args...> && !std::is_same_v<std::decay_t<Func>, winthread>>
        >
        void run(Func&& runnable, Args&&... args)
        {
            auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
            m_invoke = [runnable, args_tuple]() mutable
            {
                std::apply(runnable, args_tuple);
            };
            run();
        }

        template<
                typename Func,
                typename... Args,
                typename = std::enable_if_t<std::is_invocable_v<Func, Args...> && !std::is_same_v<std::decay_t<Func>, winthread>>
        >
        explicit winthread(Func&& runnable, Args&&... args)
        {
            auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
            m_invoke = [runnable, args_tuple]() mutable
            {
                std::apply(runnable, args_tuple);
            };
        }

    protected:

        using winthread_runnable_internal = std::function<void()>;

        bool m_is_paused { false };
        bool m_is_running { false };
        bool m_is_finished { false };

        HANDLE m_hThread { nullptr };

        priority m_priority { priority::normal };

        winthread_runnable_internal m_invoke { nullptr };

    private:

        static DWORD WINAPI thread_function(LPVOID lpParam);

    };
}