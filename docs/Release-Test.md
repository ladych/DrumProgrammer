# Abnahme und Release 1.0 (AP11)

Prüft die Qualitätsanforderungen Q-05 bis Q-11 aus Kapitel 7 des Pflichtenhefts und beschreibt, wie das Release entsteht. Ergebnisse bitte im Pull Request oder in Issue #12 festhalten.

## Automatische Messungen

Die Messtests laufen mit `ctest` in jedem optimierten Build (CI: Linux- und Windows-Job); in Debug-, Coverage- und Sanitizer-Builds werden sie übersprungen bzw. nicht gebaut.

| Anforderung | Test | Was gemessen wird | Ergebnis (Cloud-Rechner, Release-Build) |
|---|---|---|---|
| Q-05 CPU-Last ≤ 15 % | `RenderBenchmark.Q05_…` (`drumprog_benchmarks`) | Audio-Callback mit 16 gleichzeitigen, umgepitchten Stereo-Stimmen und einem 44,1-kHz-Backing-Track, der auf 48 kHz umgerechnet wird; eine Minute Audio in Blöcken zu 128 Samples | 0,1 % eines Kerns |
| Q-06 Drift = 0 Samples | `LongRunTest.Q06_…` (`drumprog_tests`) | 301 Takte (10 Minuten 2 Sekunden) Song mit Backing-Track in wechselnden, krummen Blockgrößen; Drum-Note und Klick des Backing-Tracks liegen in jedem Takt auf demselben Sample | 0 Samples Abweichung |
| Q-07 ≥ 30 fps bei 2.000 Noten | `PianoRollBenchmark.Q07_…` (`drumprog_gui_benchmarks`) | Das echte Piano-Roll des Programms mit 2.000 Noten, 120 Bilder beim Scrollen durch das Pattern, jedes Bild komplett mit dem Software-Renderer gezeichnet | rund 200 fps, langsamstes Bild 15 ms |
| Q-08 Start ≤ 3 s | von Hand (siehe unten) | Programmstart bis sichtbares Hauptfenster | erster Start 1,8 s (kopiert das Default-Kit), danach 0,5 s |

Die Werte stehen auch in der Ausgabe der Tests, z. B. `ctest --test-dir build -R Q0 -V`.

## Von Hand

Am besten mit den Paketen aus dem Release-Workflow (siehe unten), damit auch die Pakete selbst geprüft sind.

| # | Schritt | Erwartung | Anforderung |
|---|---------|-----------|-------------|
| 1 | AppImage ausführbar machen (`chmod +x`) und starten, unter Windows den Installer ausführen und das Programm aus dem Startmenü starten | Hauptfenster nach höchstens 3 Sekunden, Kit klingt | Q-08 |
| 2 | Unter Windows eine `.dpp`-Datei doppelklicken; unter Linux `./DrumProgrammer-1.0.0-x86_64.AppImage Song.dpp` | Das Projekt öffnet sich. Läuft das Programm unter Windows schon, öffnet es die Datei dort (mit Nachfrage bei ungespeicherten Änderungen); unter Linux endet der zweite Start ohne Wirkung, weil JUCE die Datei dort nicht weiterreicht | F-PJ-02 |
| 3 | Dauertest: Song mit Backing-Track 30 Minuten in der Schleife abspielen, nebenbei live einspielen; Linux mit JACK, Windows mit ASIO, Buffer 128 | Kein hörbarer Aussetzer, „XRuns: 0" in der Statusleiste | Q-03, Q-05, M4 |
| 4 | Während der Wiedergabe das MIDI-Drumkit abziehen und wieder einstecken; das Audio-Gerät in den Einstellungen wechseln | Kein Absturz, danach triggert das Kit wieder bzw. Ton auf dem neuen Gerät | Q-09 |
| 5 | Projekt öffnen, dessen Samples und Backing-Track verschoben wurden | Meldung mit den fehlenden Dateien, kein Absturz, Slots „kein Sample" | Q-09 |
| 6 | Eine Note setzen, 5 Minuten warten, dann das Programm hart beenden (Linux `kill -9 <pid>`, Windows Task-Manager „Task beenden") und neu starten | Frage „Ungespeicherte Änderungen wiederherstellen"; „Wiederherstellen" bringt die Note zurück, der Titel trägt den Stern „*" | Q-10 |
| 7 | Danach normal beenden und „Verwerfen" wählen, erneut starten | Keine Frage mehr | Q-10 |
| 8 | Eine Person, die das Programm nicht kennt, legt ohne Anleitung ein Pattern mit 2 Takten an und spielt es ab | Gelingt in höchstens 5 Minuten | Q-11 |
| 9 | Szenarien M1 bis M3 aus Kapitel 10 des Pflichtenhefts auf Windows mit ASIO (siehe [Windows-Test.md](Windows-Test.md)) | Bestanden | M4 |

Die automatische Sicherung liegt im Einstellungsordner (`~/.config/DrumProgrammer` bzw. `%APPDATA%\DrumProgrammer`) als `autosave.dpp` mit `autosave-source.txt` (zu welcher Projektdatei sie gehört). Sie wird alle 5 Minuten geschrieben, wenn sich seit der letzten Sicherung etwas geändert hat und nicht gespeichert wurde, und gelöscht, sobald nichts mehr ungespeichert ist oder das Programm normal endet.

## Release erstellen

1. Die Version steht in `CMakeLists.txt` (`project(DrumProgrammer VERSION 1.0.0 …)`); jede Version bekommt außerdem einen `<release>`-Eintrag in `packaging/linux/io.github.ladych.DrumProgrammer.metainfo.xml`.
2. Tag setzen und pushen: `git tag v1.0.0 && git push origin v1.0.0`.
3. Der Workflow „Release" baut das AppImage (auf Ubuntu 22.04, läuft also auch auf älteren Distributionen wie Linux Mint 21) und den Windows-Installer (Inno Setup) und veröffentlicht beide als GitHub-Release `v1.0.0`. Passt der Tag nicht zur Version in `CMakeLists.txt`, bricht er ab.

Pull Requests, die die Pakete ändern (`packaging/`, `tools/make-appimage.sh`, `resources/AppIcon/`, der Workflow selbst), bauen beide Pakete nur; sie liegen dann unter „Artifacts" des Laufs. Lokal entsteht das AppImage aus einem Release-Build mit `tools/make-appimage.sh build`. Das Flatpak baut der Workflow „Flatpak", die Einreichung bei Flathub beschreibt [Flathub.md](Flathub.md).
