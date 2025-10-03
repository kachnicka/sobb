#pragma once

#include <type_traits>

namespace berry {

template<typename Enum>
struct EnumValue {
    using UT = std::underlying_type_t<Enum>;
    UT value;

    EnumValue() = default;

    EnumValue(EnumValue&& rhs) = default;
    EnumValue& operator=(EnumValue&& rhs) noexcept = default;
    EnumValue(EnumValue const& rhs) = default;
    EnumValue& operator=(EnumValue const& rhs) = default;

    explicit EnumValue(Enum value)
        : value(static_cast<UT>(value)) { };

    EnumValue& operator=(Enum const& rhs)
    {
        value = static_cast<UT>(rhs);
        return *this;
    }

    operator Enum() const { return static_cast<Enum>(value); }

    UT& as_ut() { return value; }
    UT const& as_ut() const { return value; }
};

}
