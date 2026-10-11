#include "ui/AboutInfo.h"

#include <utility>

namespace drumprog::ui
{
namespace
{

ThirdPartyComponent juce()
{
    return {.name = "JUCE 8",
            .author = "Raw Material Software Limited",
            .usage = "Oberfläche, Audio- und MIDI-Treiber, Audiodateien",
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
            .usage = "FLAC-Dateien lesen (in JUCE enthalten)",
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
            .usage = "Ogg-Vorbis-Dateien lesen (in JUCE enthalten)",
            .license = "BSD-3-Clause",
            .website = {"xiph.org/vorbis", "https://xiph.org/vorbis/"},
            .licenseText = {"BSD-3-Clause", "https://opensource.org/license/bsd-3-clause"},
            .notice = {}};
}

ThirdPartyComponent zlib()
{
    return {.name = "zlib",
            .author = "Jean-loup Gailly, Mark Adler",
            .usage = "Komprimierung (in JUCE enthalten)",
            .license = "zlib License",
            .website = {"zlib.net", "https://zlib.net"},
            .licenseText = {"zlib License", "https://zlib.net/zlib_license.html"},
            .notice = {}};
}

ThirdPartyComponent libpng()
{
    return {.name = "libpng",
            .author = "Contributing Authors, Glenn Randers-Pehrson, Cosmin Truta",
            .usage = "PNG-Bilder (in JUCE enthalten)",
            .license = "PNG Reference Library License",
            .website = {"libpng.org", "http://www.libpng.org/pub/png/libpng.html"},
            .licenseText = {"libpng License", "http://www.libpng.org/pub/png/src/libpng-LICENSE.txt"},
            .notice = {}};
}

ThirdPartyComponent ijgJpeg()
{
    return {.name = "IJG JPEG Library",
            .author = "Independent JPEG Group",
            .usage = "JPEG-Bilder (in JUCE enthalten)",
            .license = "IJG License",
            .website = {"ijg.org", "https://ijg.org"},
            .licenseText = {"IJG License", "https://ijg.org/files/README"},
            .notice = "This software is based in part on the work of the Independent JPEG Group."};
}

ThirdPartyComponent asioSdk()
{
    return {.name = "ASIO SDK 2.3.4",
            .author = "Steinberg Media Technologies GmbH",
            .usage = "ASIO-Treiber unter Windows",
            .license = "GNU General Public License v3.0",
            .website = {"steinberg.net/developers", "https://www.steinberg.net/developers/"},
            .licenseText = {"GPLv3", "https://www.gnu.org/licenses/gpl-3.0.html"},
            .notice = "ASIO is a trademark and software of Steinberg Media Technologies GmbH."};
}

ThirdPartyComponent blackPearl()
{
    return {.name = "Black Pearl (AVL Drumkits)",
            .author = "Glen MacArthur",
            .usage = "Samples des Default-Kits, je Instrument die höchste Velocity-Stufe, unverändert",
            .license = "Creative Commons Namensnennung - Weitergabe unter gleichen Bedingungen 3.0",
            .website = {"bandshed.net/avldrumkits", "http://www.bandshed.net/avldrumkits/"},
            .licenseText = {"CC BY-SA 3.0", "https://creativecommons.org/licenses/by-sa/3.0/deed.de"},
            .notice = {}};
}

/// Icons of the buttons (AP14). Some of them come from Feather, whose MIT license asks for its notice too.
ThirdPartyComponent lucide()
{
    return {.name = "Lucide Icons 1.55.0",
            .author = "Lucide Icons and Contributors",
            .usage = "Icons der Buttons, unverändert",
            .license = "ISC License, einzelne Icons aus Feather unter MIT License",
            .website = {"lucide.dev", "https://lucide.dev"},
            .licenseText = {"ISC", "https://lucide.dev/license"},
            .notice = "Copyright (c) 2026 Lucide Icons and Contributors. "
                      "Teile aus Feather: Copyright (c) 2013-present Cole Bemis (MIT License)."};
}

} // namespace

AboutInfo makeAboutInfo(std::string_view version, bool withAsio)
{
    AboutInfo info;
    info.title = "Drum Programmer " + std::string{version};
    info.lines.emplace_back("Schlagzeug-Patterns programmieren, einspielen und als MIDI exportieren.");
    info.lines.emplace_back("Freie Software unter der GNU Affero General Public License v3.0, ohne jede "
                            "Gewährleistung. Der Quellcode ist öffentlich.");
    info.lines.emplace_back("Erstellt mit Claude (Anthropic) und Claude Code.");
    info.links.push_back({"Quellcode auf GitHub", "https://github.com/ladych/DrumProgrammer"});
    info.links.push_back({"Lizenz AGPLv3", "https://www.gnu.org/licenses/agpl-3.0.html"});
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
