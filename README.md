# Drum Programmer

Desktop-Anwendung zum Programmieren von Schlagzeug-Patterns (JUCE/C++), mit Echtzeit-Eingabe über Tastatur und MIDI-Hardware, Piano-Roll-Editor, Song-Arrangement und Backing-Track. Zielplattformen: Linux (ALSA/JACK) und Windows (ASIO).

## Planung

- [Lastenheft](docs/Lastenheft_Drum_Programmer.md)
- [Pflichtenheft](docs/Pflichtenheft_Drum_Programmer.md) – Arbeitspakete in Kapitel 8, Meilensteine in Kapitel 9
- [GUI-Entwurf v0.1](docs/GUI-Entwurf_v0.1.png)
- [Latenzmessung](docs/Latenzmessung.md) – Vorgehen für Q-01 und Q-02
- Fortschritt: Issues und Meilensteine in diesem Repository

## Bauen

Voraussetzungen: CMake ≥ 3.22, Ninja, ein C++20-Compiler (GCC/Clang unter Linux, Visual Studio (MSVC) unter Windows). Unter Linux zusätzlich die JUCE-Abhängigkeiten:

```sh
sudo apt install libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libgl1-mesa-dev
```

JUCE und GoogleTest liegen als Git-Submodule unter `external/`:

```sh
git clone --recurse-submodules https://github.com/ladych/DrumProgrammer.git
# bei bestehendem Klon: git submodule update --init --recursive
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

**Windows mit ASIO:** Das Steinberg-ASIO-SDK darf nicht ins Repository (Q-12). Es wird separat [von Steinberg](https://www.steinberg.net/developers/) geladen, entpackt und beim Konfigurieren angegeben: `cmake -S . -B build -A x64 -DDRUMPROG_ASIO_SDK_DIR=C:/pfad/zum/asiosdk` (das Verzeichnis mit `common/iasiodrv.h`). Ohne SDK bietet der Windows-Build nur WASAPI und DirectSound. In der CI lädt der Windows-Job das SDK von der URL im Repository-Secret `ASIO_SDK_URL` und stellt das fertige Programm als Artefakt `DrumProgrammer-Windows` bereit.

Beim ersten Start wählt das Programm ASIO, falls ein ASIO-Treiber installiert ist, sonst WASAPI (unter Linux ALSA). Fällt das gewählte Gerät aus (z. B. Interface abgezogen), spielt es über den nächsten Treiber weiter und kehrt zum gewählten Gerät zurück, sobald es wieder da ist; die Statusleiste zeigt das an.

CLion liest `CMakePresets.json` direkt; die Presets `coverage`, `asan` und `tsan` bauen nur die Tests.

## Qualitätsregeln

- Tests mit GoogleTest/GoogleMock, Testnamen mit Anforderungs-ID (E-01, E-09)
- 100 % Zeilen- und Branch-Abdeckung für `src/`, Ausnahmen mit Begründung in `coverage-exclusions.txt` (E-02, E-03); lokal: `cmake --preset coverage && cmake --build --preset coverage && ctest --preset coverage && tools/check-coverage.py build/coverage`
- `clang-format` und `tools/run-clang-tidy.sh <build-dir>` ohne Befund, Warnungen sind Fehler (E-08)
- Composition Root in `src/app/AppComposition.cpp` (E-05)

Die CI (GitHub Actions) prüft all das bei jedem Push: Linux- und Windows-Build mit Tests, Coverage-Gate, Address/UB- und Thread-Sanitizer, clang-format und clang-tidy.

## Lizenz

GNU Affero General Public License v3.0, siehe [LICENSE](LICENSE). Die Lizenz folgt aus der Open-Source-Lizenz von JUCE.
