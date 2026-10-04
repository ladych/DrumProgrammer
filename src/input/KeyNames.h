#pragma once

#include <string>

namespace drumprog::input
{

/// Label of a physical key as printed on the user's keyboard, e.g. "Z" for scancode kY on a German
/// keyboard. The platform implementation asks the operating system for the current layout.
class IKeyNames
{
public:
    virtual ~IKeyNames() = default;

    [[nodiscard]] virtual std::string nameOf(int scancode) const = 0;

protected:
    IKeyNames() = default;
    IKeyNames(const IKeyNames&) = default;
    IKeyNames(IKeyNames&&) = default;
    IKeyNames& operator=(const IKeyNames&) = default;
    IKeyNames& operator=(IKeyNames&&) = default;
};

/// Labels of the US layout, used where the operating system cannot name a key.
class UsKeyNames final : public IKeyNames
{
public:
    /// "Taste <n>" for keys outside the table.
    [[nodiscard]] std::string nameOf(int scancode) const override;
};

} // namespace drumprog::input
