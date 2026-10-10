#include "ui/MixPresenter.h"

#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

class MixPresenterTest : public ::testing::Test
{
protected:
    [[nodiscard]] model::Mix mix() const { return model::Project{tree, nullptr}.mix(); }

    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::UndoManager undoManager;
    MixPresenter presenter{tree, undoManager};
};

TEST_F(MixPresenterTest, FBT06_ThreeFadersStartAtZeroDecibels)
{
    EXPECT_EQ(MixPresenter::label(MixChannel::backing), "Backing");
    EXPECT_EQ(MixPresenter::label(MixChannel::drums), "Drums");
    EXPECT_EQ(MixPresenter::label(MixChannel::master), "Master");
    for (const auto channel : MixPresenter::kChannels)
        EXPECT_DOUBLE_EQ(presenter.gainDb(channel), 0.0);
}

TEST_F(MixPresenterTest, FBT06_EachFaderSetsItsOwnGain)
{
    const auto changes = presenter.changeCount();
    presenter.setGainDb(MixChannel::backing, -6.0);
    presenter.setGainDb(MixChannel::drums, -20.0);
    presenter.setGainDb(MixChannel::master, 6.0);
    EXPECT_GT(presenter.changeCount(), changes);
    EXPECT_NEAR(mix().backingGain(), 0.501, 0.001);
    EXPECT_DOUBLE_EQ(mix().drumsGain(), 0.1);
    EXPECT_NEAR(mix().masterGain(), 1.995, 0.001);
    EXPECT_DOUBLE_EQ(presenter.gainDb(MixChannel::drums), -20.0);
}

TEST_F(MixPresenterTest, FBT06_GainsAreClampedAndTheBottomIsSilence)
{
    presenter.setGainDb(MixChannel::master, 20.0);
    EXPECT_DOUBLE_EQ(presenter.gainDb(MixChannel::master), MixPresenter::kMaxGainDb);
    presenter.setGainDb(MixChannel::drums, -100.0);
    EXPECT_EQ(mix().drumsGain(), 0.0);
    EXPECT_EQ(presenter.gainDb(MixChannel::drums), MixPresenter::kMinGainDb);
    model::Project{tree, nullptr}.mix().setBackingGain(0.0000001);
    EXPECT_EQ(presenter.gainDb(MixChannel::backing), MixPresenter::kMinGainDb);
}

TEST_F(MixPresenterTest, FPJ05_MovingOneFaderIsOneUndoStep)
{
    presenter.setGainDb(MixChannel::drums, -1.0);
    presenter.setGainDb(MixChannel::drums, -2.0);
    presenter.setGainDb(MixChannel::backing, -3.0);
    undoManager.undo();
    EXPECT_DOUBLE_EQ(presenter.gainDb(MixChannel::backing), 0.0);
    EXPECT_DOUBLE_EQ(presenter.gainDb(MixChannel::drums), -2.0);
    undoManager.undo();
    EXPECT_DOUBLE_EQ(presenter.gainDb(MixChannel::drums), 0.0);
}

} // namespace
} // namespace drumprog::ui
