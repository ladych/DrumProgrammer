#include "ui/AboutInfo.h"

#include "i18n/Translate.h"

#include <utility>

namespace drumprog::ui
{
namespace
{

ThirdPartyComponent juce()
{
    return {.name = "JUCE 8",
            .author = "Raw Material Software Limited",
            .usage = i18n::tr("User interface, audio and MIDI drivers, audio files"),
            .license = "GNU Affero General Public License v3.0",
            .website = {"juce.com", "https://juce.com"},
            .licenseText = {"AGPLv3", "https://www.gnu.org/licenses/agpl-3.0.html"},
            .notice = {}};
}

// Libraries JUCE builds into the program. They are appended one by one: a brace-initialized vector of
// these structs leaves exception branches no test can reach (coverage gate, E-02).

/// Reads FLAC files.
ThirdPartyComponent flac()
{
    return {.name = "FLAC",
            .author = "Josh Coalson, Xiph.Org Foundation",
            .usage = i18n::tr("Reading FLAC files (included in JUCE)"),
            .license = "BSD-3-Clause",
            .website = {"xiph.org/flac", "https://xiph.org/flac/"},
            .licenseText = {"BSD-3-Clause", "https://opensource.org/license/bsd-3-clause"},
            .notice = {}};
}

/// Reads Ogg Vorbis files.
ThirdPartyComponent oggVorbis()
{
    return {.name = "Ogg Vorbis",
            .author = "Xiph.Org Foundation",
            .usage = i18n::tr("Reading Ogg Vorbis files (included in JUCE)"),
            .license = "BSD-3-Clause",
            .website = {"xiph.org/vorbis", "https://xiph.org/vorbis/"},
            .licenseText = {"BSD-3-Clause", "https://opensource.org/license/bsd-3-clause"},
            .notice = {}};
}

ThirdPartyComponent zlib()
{
    return {.name = "zlib",
            .author = "Jean-loup Gailly, Mark Adler",
            .usage = i18n::tr("Compression (included in JUCE)"),
            .license = "zlib License",
            .website = {"zlib.net", "https://zlib.net"},
            .licenseText = {"zlib License", "https://zlib.net/zlib_license.html"},
            .notice = {}};
}

ThirdPartyComponent libpng()
{
    return {.name = "libpng",
            .author = "Contributing Authors, Glenn Randers-Pehrson, Cosmin Truta",
            .usage = i18n::tr("PNG images (included in JUCE)"),
            .license = "PNG Reference Library License",
            .website = {"libpng.org", "http://www.libpng.org/pub/png/libpng.html"},
            .licenseText = {"libpng License", "http://www.libpng.org/pub/png/src/libpng-LICENSE.txt"},
            .notice = {}};
}

ThirdPartyComponent ijgJpeg()
{
    return {.name = "IJG JPEG Library",
            .author = "Independent JPEG Group",
            .usage = i18n::tr("JPEG images (included in JUCE)"),
            .license = "IJG License",
            .website = {"ijg.org", "https://ijg.org"},
            .licenseText = {"IJG License", "https://ijg.org/files/README"},
            .notice = "This software is based in part on the work of the Independent JPEG Group."};
}

ThirdPartyComponent asioSdk()
{
    return {.name = "ASIO SDK 2.3.4",
            .author = "Steinberg Media Technologies GmbH",
            .usage = i18n::tr("ASIO drivers on Windows"),
            .license = "GNU General Public License v3.0",
            .website = {"steinberg.net/developers", "https://www.steinberg.net/developers/"},
            .licenseText = {"GPLv3", "https://www.gnu.org/licenses/gpl-3.0.html"},
            .notice = "ASIO is a trademark and software of Steinberg Media Technologies GmbH."};
}

ThirdPartyComponent blackPearl()
{
    return {
        .name = "Black Pearl (AVL Drumkits)",
        .author = "Glen MacArthur",
        .usage =
            i18n::tr("Samples of the default kit, the highest velocity layer of each instrument, unchanged"),
        .license = i18n::tr("Creative Commons Attribution-ShareAlike 3.0"),
        .website = {"bandshed.net/avldrumkits", "http://www.bandshed.net/avldrumkits/"},
        .licenseText = {"CC BY-SA 3.0", i18n::tr("https://creativecommons.org/licenses/by-sa/3.0/deed.en")},
        .notice = {}};
}

/// Icons of the buttons (AP14). Some of them come from Feather, whose MIT license asks for its notice too.
ThirdPartyComponent lucide()
{
    return {.name = "Lucide Icons 1.55.0",
            .author = "Lucide Icons and Contributors",
            .usage = i18n::tr("Icons of the buttons, unchanged"),
            .license = i18n::tr("ISC License, some icons from Feather under the MIT License"),
            .website = {"lucide.dev", "https://lucide.dev"},
            .licenseText = {"ISC", "https://lucide.dev/license"},
            .notice = i18n::tr("Copyright (c) 2026 Lucide Icons and Contributors. "
                               "Parts from Feather: Copyright (c) 2013-present Cole Bemis (MIT License).")};
}

} // namespace

AboutInfo makeAboutInfo(std::string_view version, bool withAsio)
{
    AboutInfo info;
    info.title = "Drum Programmer " + std::string{version};
    info.lines.push_back(i18n::tr("Program drum patterns, play them in and export them as MIDI."));
    info.lines.push_back(
        i18n::tr("Free software under the GNU Affero General Public License v3.0, without any "
                 "warranty. The source code is public."));
    info.lines.push_back(i18n::tr("Made with Claude (Anthropic) and Claude Code."));
    info.links.push_back({i18n::tr("Source code on GitHub"), "https://github.com/ladych/DrumProgrammer"});
    info.links.push_back({i18n::tr("License AGPLv3"), "https://www.gnu.org/licenses/agpl-3.0.html"});
    info.links.push_back({"Claude Code", "https://claude.com/claude-code"});
    for (auto* const make : {&juce, &flac, &oggVorbis, &zlib, &libpng, &ijgJpeg})
        info.components.push_back(make());
    if (withAsio)
        info.components.push_back(asioSdk());
    info.components.push_back(blackPearl());
    info.components.push_back(lucide());
    return info;
}

} // namespace drumprog::ui
