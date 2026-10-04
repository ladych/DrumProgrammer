#include "ui/TempoPresenter.h"

#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace drumprog::ui
{
namespace
{

class TempoPresenterTest : public ::testing::Test
{
protected:
    model::FakeIdGenerator ids;
    juce::ValueTree tree = model::ProjectFactory{ids}.createDefault();
    juce::UndoManager undoManager;
    TempoPresenter tempo{tree, undoManager};
};

TEST_F(TempoPresenterTest, FTR03_ShowsTheProjectTempoWithTwoDecimals)
{
    EXPECT_DOUBLE_EQ(tempo.bpm(), 120.0);
    EXPECT_EQ(tempo.bpmText(), "120.00");
    tempo.setBpm(97.456);
    EXPECT_EQ(tempo.bpmText(), "97.46");
    EXPECT_DOUBLE_EQ(model::Project(tree, nullptr).bpm(), 97.46);
}

TEST_F(TempoPresenterTest, FTR03_ClampsTheTempoTo30To300)
{
    tempo.setBpm(10.0);
    EXPECT_DOUBLE_EQ(tempo.bpm(), 30.0);
    tempo.setBpm(400.0);
    EXPECT_DOUBLE_EQ(tempo.bpm(), 300.0);
}

TEST_F(TempoPresenterTest, FTR03_ParsesTypedTempo)
{
    EXPECT_TRUE(tempo.setBpmText("97,5"));
    EXPECT_DOUBLE_EQ(tempo.bpm(), 97.5);
    EXPECT_TRUE(tempo.setBpmText(" 140.25 "));
    EXPECT_DOUBLE_EQ(tempo.bpm(), 140.25);
    EXPECT_FALSE(tempo.setBpmText("schnell"));
    EXPECT_FALSE(tempo.setBpmText("90x"));
    EXPECT_DOUBLE_EQ(tempo.bpm(), 140.25);
}

TEST_F(TempoPresenterTest, FTR03_SetsValidTimeSignatures)
{
    EXPECT_EQ(tempo.timeSignatureText(), "4/4");
    tempo.setTimeSignature(6, 8);
    EXPECT_EQ(tempo.timeSignatureText(), "6/8");
    tempo.setTimeSignature(20, 16);
    EXPECT_EQ(tempo.timeSignatureText(), "16/16");
    tempo.setTimeSignature(0, 4);
    EXPECT_EQ(tempo.timeSignatureText(), "1/4");
    tempo.setTimeSignature(3, 2);
    EXPECT_EQ(tempo.timeSignatureText(), "1/4");
    EXPECT_EQ(tempo.timeSignature().numerator, 1);
}

TEST_F(TempoPresenterTest, FTR03_FPJ05_EachChangeIsOneUndoStep)
{
    tempo.setBpm(90.0);
    tempo.setTimeSignature(3, 4);
    tempo.setBpm(100.0);
    ASSERT_TRUE(undoManager.undo());
    EXPECT_DOUBLE_EQ(tempo.bpm(), 90.0);
    ASSERT_TRUE(undoManager.undo());
    EXPECT_EQ(tempo.timeSignatureText(), "4/4");
    ASSERT_TRUE(undoManager.undo());
    EXPECT_DOUBLE_EQ(tempo.bpm(), 120.0);
}

TEST_F(TempoPresenterTest, FTR03_OnlyQuarterEighthAndSixteenthBeats)
{
    EXPECT_TRUE(TempoPresenter::isValidDenominator(4));
    EXPECT_TRUE(TempoPresenter::isValidDenominator(8));
    EXPECT_TRUE(TempoPresenter::isValidDenominator(16));
    EXPECT_FALSE(TempoPresenter::isValidDenominator(2));
}

} // namespace
} // namespace drumprog::ui
