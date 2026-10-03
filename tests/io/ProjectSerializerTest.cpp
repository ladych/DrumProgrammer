#include "io/ProjectSerializer.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace drumprog::io
{
namespace
{

namespace fs = std::filesystem;
namespace ids = model::ids;
using model::NoteOrigin;
using model::Project;

/// An absolute directory on every platform ("/projects" has no drive letter on Windows).
fs::path absoluteDirectory(const fs::path& relative)
{
    return fs::current_path().root_path() / relative;
}

std::string generic(const fs::path& path)
{
    return path.generic_string();
}

class ProjectSerializerTest : public ::testing::Test
{
protected:
    ParsedProject roundTrip() const
    {
        return ProjectSerializer::fromXml(ProjectSerializer::toXml(tree, projectDir), projectDir);
    }

    model::FakeIdGenerator idGenerator;
    juce::ValueTree tree = model::ProjectFactory{idGenerator}.createDefault();
    Project project{tree, nullptr};
    fs::path projectDir = absoluteDirectory("projects/rock");
};

TEST_F(ProjectSerializerTest, FPJ02_WritesXmlWithFormatVersion)
{
    const auto xml = juce::parseXML(juce::String{ProjectSerializer::toXml(tree, projectDir)});

    ASSERT_NE(xml, nullptr);
    EXPECT_TRUE(xml->hasTagName("PROJECT"));
    EXPECT_EQ(xml->getIntAttribute("formatVersion"), 1);
}

TEST_F(ProjectSerializerTest, FPJ02_RoundTripKeepsTheWholeProject)
{
    project.setName("Rock-Demo");
    project.setBpm(97.5);
    project.setTimeSignature({7, 8});
    project.pattern(0).addNote({38, 960, 240, 90, NoteOrigin::live});
    project.addPattern("verse", "Verse", 4).setColour("#3B82F6");
    project.song().addEntry("verse", 2);
    project.mix().setMasterGain(0.75);
    project.kit().findSlot(38)->setGain(0.5);

    const auto parsed = roundTrip();

    ASSERT_EQ(parsed.error, ProjectFileError::none);
    const Project loaded{parsed.project, nullptr};
    EXPECT_EQ(loaded.name(), "Rock-Demo");
    EXPECT_DOUBLE_EQ(loaded.bpm(), 97.5);
    EXPECT_EQ(loaded.timeSignature(), (model::TimeSignature{7, 8}));
    EXPECT_EQ(loaded.pattern(0).note(0).data(), (model::NoteData{38, 960, 240, 90, NoteOrigin::live}));
    EXPECT_EQ(loaded.findPattern("verse")->colour(), "#3B82F6");
    EXPECT_EQ(loaded.song().entry(0).startBar(), 2);
    EXPECT_DOUBLE_EQ(loaded.mix().masterGain(), 0.75);
    EXPECT_DOUBLE_EQ(loaded.kit().findSlot(38)->gain(), 0.5);
    EXPECT_EQ(loaded.kit().numSlots(), 25);
}

TEST_F(ProjectSerializerTest, FPJ02_DoesNotChangeTheProjectInMemory)
{
    const auto before = tree.createCopy();
    project.kit().findSlot(36)->setFilePath(generic(projectDir / "samples/kick.wav"));
    const auto withPath = tree.createCopy();

    static_cast<void>(ProjectSerializer::toXml(tree, projectDir));

    EXPECT_TRUE(tree.isEquivalentTo(withPath));
    EXPECT_FALSE(tree.isEquivalentTo(before));
}

TEST_F(ProjectSerializerTest, FPJ02_StoresSamplePathsRelativeToTheProjectFile)
{
    project.kit().findSlot(36)->setFilePath((projectDir / "samples" / "kick.wav").string());
    project.kit().findSlot(38)->setFilePath((projectDir.parent_path() / "shared" / "snare.wav").string());
    project.backingTrack().setFilePath((projectDir / "song.wav").string());

    const auto xml = juce::parseXML(juce::String{ProjectSerializer::toXml(tree, projectDir)});

    const auto* kit = xml->getChildByName("KIT");
    EXPECT_EQ(kit->getChildElement(1)->getStringAttribute("filePath"), "samples/kick.wav");
    EXPECT_EQ(kit->getChildElement(3)->getStringAttribute("filePath"), "../shared/snare.wav");
    EXPECT_EQ(xml->getChildByName("BACKING_TRACK")->getStringAttribute("filePath"), "song.wav");
}

TEST_F(ProjectSerializerTest, FPJ02_ResolvesRelativePathsAgainstTheNewLocation)
{
    project.kit().findSlot(36)->setFilePath((projectDir / "samples" / "kick.wav").string());
    const auto xml = ProjectSerializer::toXml(tree, projectDir);
    const auto movedDir = absoluteDirectory("backup/rock");

    const auto parsed = ProjectSerializer::fromXml(xml, movedDir);

    const Project loaded{parsed.project, nullptr};
    EXPECT_EQ(fs::path{loaded.kit().findSlot(36)->filePath()}, movedDir / "samples" / "kick.wav");
}

TEST_F(ProjectSerializerTest, FPJ02_EmptyPathsStayEmpty)
{
    const auto parsed = roundTrip();

    const Project loaded{parsed.project, nullptr};
    EXPECT_EQ(loaded.kit().findSlot(36)->filePath(), "");
    EXPECT_EQ(loaded.backingTrack().filePath(), "");
}

TEST_F(ProjectSerializerTest, FPJ02_KeepsPathsThatCannotBeMadeRelative)
{
    // Like a path on another drive on Windows: std::filesystem finds no relative path to it.
    project.kit().findSlot(36)->setFilePath("samples/kick.wav");

    const auto xml = juce::parseXML(juce::String{ProjectSerializer::toXml(tree, projectDir)});

    EXPECT_EQ(xml->getChildByName("KIT")->getChildElement(1)->getStringAttribute("filePath"),
              "samples/kick.wav");
}

TEST_F(ProjectSerializerTest, FPJ02_KeepsAbsolutePathsFromTheFile)
{
    const auto absolute = generic(absoluteDirectory("samples/kick.wav"));
    auto xml = juce::parseXML(juce::String{ProjectSerializer::toXml(tree, projectDir)});
    xml->getChildByName("KIT")->getChildElement(1)->setAttribute("filePath", juce::String{absolute});

    const auto parsed = ProjectSerializer::fromXml(xml->toString().toStdString(), projectDir);

    EXPECT_EQ(fs::path{Project(parsed.project, nullptr).kit().findSlot(36)->filePath()}, fs::path{absolute});
}

TEST_F(ProjectSerializerTest, FPJ03_DoesNotWriteTheMissingSampleMarker)
{
    tree.getChildWithName(ids::kit).getChild(1).setProperty(ids::sampleMissing, true, nullptr);

    const auto xml = ProjectSerializer::toXml(tree, projectDir);

    EXPECT_EQ(xml.find("sampleMissing"), std::string::npos);
}

TEST_F(ProjectSerializerTest, FPJ02_RejectsTextThatIsNoXml)
{
    EXPECT_EQ(ProjectSerializer::fromXml("not xml", projectDir).error, ProjectFileError::notAProject);
}

TEST_F(ProjectSerializerTest, FPJ02_RejectsOtherXml)
{
    EXPECT_EQ(ProjectSerializer::fromXml("<DEVICESETUP/>", projectDir).error, ProjectFileError::notAProject);
}

TEST_F(ProjectSerializerTest, FPJ02_RejectsProjectWithoutFormatVersion)
{
    EXPECT_EQ(ProjectSerializer::fromXml("<PROJECT/>", projectDir).error, ProjectFileError::notAProject);
}

TEST_F(ProjectSerializerTest, FPJ02_RejectsNewerFormatVersion)
{
    EXPECT_EQ(ProjectSerializer::fromXml(R"(<PROJECT formatVersion="2"/>)", projectDir).error,
              ProjectFileError::newerFormatVersion);
}

TEST_F(ProjectSerializerTest, FPJ02_FillsInMissingSectionsSoTheModelIsComplete)
{
    const auto parsed = ProjectSerializer::fromXml(R"(<PROJECT formatVersion="1" bpm="100"/>)", projectDir);

    ASSERT_EQ(parsed.error, ProjectFileError::none);
    EXPECT_TRUE(parsed.project.getChildWithName(ids::kit).isValid());
    EXPECT_TRUE(parsed.project.getChildWithName(ids::patterns).isValid());
    EXPECT_TRUE(parsed.project.getChildWithName(ids::song).isValid());
    EXPECT_TRUE(parsed.project.getChildWithName(ids::backingTrack).isValid());
    EXPECT_TRUE(parsed.project.getChildWithName(ids::mix).isValid());
    EXPECT_DOUBLE_EQ(Project(parsed.project, nullptr).bpm(), 100.0);
}

} // namespace
} // namespace drumprog::io
