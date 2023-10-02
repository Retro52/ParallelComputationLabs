#pragma once

#include <random>

namespace retro::core
{
    class random
    {
    public:

        template<typename T>
        static T generate(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
        {
            if constexpr (std::is_integral_v<T>)
            {
                std::uniform_int_distribution<T> distribution(min, max);
                return distribution(m_engine);
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                std::uniform_real_distribution<T> distribution(min, max);
                return distribution(m_engine);
            }
            else
            {
                static_assert(false, "Type not supported for random generation");
            }
        }

    private:
        inline thread_local static std::mt19937 m_engine { std::random_device{}() };
    };
}
