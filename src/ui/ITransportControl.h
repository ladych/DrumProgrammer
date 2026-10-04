#pragma once

namespace drumprog::ui
{

/// What the keyboard can do with the transport: the space bar starts and stops (F-TR-01).
class ITransportControl
{
public:
    virtual ~ITransportControl() = default;

    virtual void togglePlay() = 0;

protected:
    ITransportControl() = default;
    ITransportControl(const ITransportControl&) = default;
    ITransportControl(ITransportControl&&) = default;
    ITransportControl& operator=(const ITransportControl&) = default;
    ITransportControl& operator=(ITransportControl&&) = default;
};

} // namespace drumprog::ui
