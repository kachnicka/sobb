#pragma once

#include <array>
#include <cstddef>

namespace berry {

template<typename Enum, typename T, size_t Size = static_cast<size_t>(Enum::eCount)>
struct ConstexprEnumMap {
    std::array<T, Size> data {};

    constexpr T const& operator[](Enum key) const { return data[static_cast<size_t>(key)]; }
    constexpr T& operator[](Enum key) { return data[static_cast<size_t>(key)]; }
    constexpr auto begin() { return data.begin(); }
    constexpr auto end() { return data.end(); }
    constexpr auto begin() const { return data.begin(); }
    constexpr auto end() const { return data.end(); }
    constexpr auto cbegin() const { return data.cbegin(); }
    constexpr auto cend() const { return data.cend(); }
};

}
