#include "engine/SampleEngine.h"

#include "engine/RenderHelpers.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <new>

namespace
{

// Counts heap allocations of the current thread while counting is switched on.
thread_local bool countAllocations = false;
thread_local int allocationCount = 0;

} // namespace

void* operator new(std::size_t size)
{
    if (countAllocations)
        ++allocationCount;
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t /*size*/) noexcept
{
    std::free(memory);
}

namespace drumprog::engine
{
namespace
{

std::unique_ptr<EngineKit> makeKit()
{
    auto sample = std::make_shared<const SampleBuffer>(SampleBuffer{48000.0, {std::vector<float>(5000, 0.1F)}});
    return std::make_unique<EngineKit>(
        std::vector<EngineSlot>{EngineSlot{.slotIndex = 0, .midiNote = 42, .sample = sample, .chokeGroup = 1},
                                EngineSlot{.slotIndex = 1, .midiNote = 46, .sample = sample, .chokeGroup = 1},
                                EngineSlot{.slotIndex = 2, .midiNote = 36, .sample = sample}});
}

TEST(RealtimeAllocationTest, Q04_RenderDoesNotAllocate)
{
    SampleEngine engine;
    engine.prepare(48000.0);
    engine.setKit(makeKit());
    StereoOutput out(128);

    countAllocations = true;
    for (int block = 0; block < 200; ++block)
    {
        for (int hit = 0; hit < 3; ++hit)
            engine.trigger(36, 100, hit * 10); // overflows the voice pool
        engine.trigger(block % 2 == 0 ? 46 : 42, 100, 5);
        engine.render(out.channels.data(), 2, 128);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
}

TEST(RealtimeAllocationTest, Q04_KitSwapAndQueuedTriggersDoNotAllocateOnAudioThread)
{
    SampleEngine engine;
    engine.prepare(48000.0);
    engine.setKit(makeKit());
    StereoOutput out(128);
    engine.render(out.channels.data(), 2, 128);

    for (int block = 0; block < 20; ++block)
    {
        engine.setKit(makeKit());
        engine.queueTrigger(36, 100);
        countAllocations = true;
        engine.render(out.channels.data(), 2, 128);
        countAllocations = false;
        engine.collectGarbage();
    }

    EXPECT_EQ(allocationCount, 0);
}

} // namespace
} // namespace drumprog::engine
