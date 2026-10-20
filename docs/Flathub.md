# Flathub

Drum Programmer wird auf [Flathub](https://flathub.org) unter der App-ID `io.github.ladych.DrumProgrammer` veröffentlicht (AP24). Die ID folgt der Flathub-Regel für Projekte auf GitHub: `io.github.<Benutzer>.<Repository>`; Flathub prüft sie über das GitHub-Konto `ladych`.

## Dateien

| Datei | Zweck |
|---|---|
| `packaging/flatpak/io.github.ladych.DrumProgrammer.yml` | Flatpak-Manifest, baut das ausgecheckte Repository |
| `packaging/linux/io.github.ladych.DrumProgrammer.desktop` | Startmenü-Eintrag (auch im AppImage) |
| `packaging/linux/io.github.ladych.DrumProgrammer.metainfo.xml` | AppStream-Beschreibung für Flathub und Software-Center: Text, Screenshot, Versionen |
| `packaging/linux/io.github.ladych.DrumProgrammer.mime.xml` | Dateityp `.dpp`, damit ein Doppelklick das Projekt öffnet |
| `tools/make-flathub-manifest.sh` | Erzeugt aus dem Manifest die Fassung für Flathub mit einem Release-Tag als Quelle |

`cmake --install build` legt Programm (`bin/drumprogrammer`), Startmenü-Eintrag, Metainfo, Dateityp, Icon und Lizenz so ab, wie Flatpak und AppImage sie brauchen.

Das Flatpak läuft mit der Freedesktop-Runtime. Es zeichnet über X11 (unter Wayland über XWayland), spielt über ALSA (in der Runtime an PulseAudio/PipeWire angebunden) oder JACK (über PipeWire) und liest MIDI-Controller über den ALSA-Sequencer, dafür braucht es `--device=all`. Weil JUCE keine Dateiauswahl über Portale kennt, darf es den Home-Ordner lesen und schreiben. Die Einstellungen liegen im Flatpak unter `~/.var/app/io.github.ladych.DrumProgrammer/config/DrumProgrammer`.

## Bauen und testen

Der Workflow „Flatpak" baut das Paket bei Pull Requests, die die Pakete ändern, und bei jedem Tag `v*`, prüft es mit dem Flathub-Linter und legt `DrumProgrammer.flatpak` unter „Artifacts" ab. Installieren und starten:

```sh
flatpak install --user DrumProgrammer.flatpak
flatpak run io.github.ladych.DrumProgrammer
```

Lokal (mit `flatpak-builder` und dem Flathub-Remote):

```sh
flatpak install --user flathub org.freedesktop.Sdk//26.08 org.freedesktop.Platform//26.08
flatpak-builder --user --install --force-clean build-flatpak packaging/flatpak/io.github.ladych.DrumProgrammer.yml
flatpak run io.github.ladych.DrumProgrammer
```

## Erste Einreichung

Das macht der Inhaber des GitHub-Kontos `ladych` selbst, weil die Einreichung eine Veröffentlichung ist und Flathub den Antragsteller als Betreuer einträgt.

1. Release mit den Flatpak-Dateien erstellen (siehe [Release-Test.md](Release-Test.md)), z. B. `v1.0.1`. Der Screenshot in der Metainfo verweist auf `docs/MainWindow.png` in genau diesem Tag; bei einer neuen Version die URL und den Eintrag unter `<releases>` anpassen.
2. Auf GitHub [flathub/flathub](https://github.com/flathub/flathub) forken, dabei „Copy the master branch only" abwählen.
3. Im Fork vom Branch `new-pr` einen eigenen Branch anlegen und dort die Flathub-Fassung des Manifests ablegen:
   ```sh
   git clone --branch new-pr https://github.com/ladych/flathub.git && cd flathub
   git checkout -b io.github.ladych.DrumProgrammer
   ../DrumProgrammer/tools/make-flathub-manifest.sh v1.0.1 io.github.ladych.DrumProgrammer.yml
   git add io.github.ladych.DrumProgrammer.yml && git commit -m "Add io.github.ladych.DrumProgrammer" && git push -u origin HEAD
   ```
4. Pull Request gegen **`new-pr`** (nicht `master`) von flathub/flathub öffnen, Titel `Add io.github.ladych.DrumProgrammer`, die Checkliste der Vorlage ausfüllen.
5. Im PR mit dem Kommentar `bot, build` einen Testbau starten. Die Reviewer prüfen Manifest, Rechte (`finish-args`) und Metainfo und stellen eventuell Rückfragen, z. B. zu `--device=all` und `--filesystem=home` (Begründung steht im Manifest).
6. Nach der Annahme legt Flathub das Repository `flathub/io.github.ladych.DrumProgrammer` an und lädt `ladych` als Betreuer ein. Dafür muss auf GitHub die Zwei-Faktor-Authentifizierung aktiv sein. Einige Stunden nach dem ersten Bau erscheint die App auf flathub.org.
7. Optional auf flathub.org mit GitHub anmelden und die App als „verifiziert" bestätigen (Developer Portal → Verification).

## Neue Version

1. Version in `CMakeLists.txt` und einen `<release>`-Eintrag in der Metainfo anlegen, Tag pushen.
2. Im Repository `flathub/io.github.ladych.DrumProgrammer` in der Manifest-Quelle `tag` und `commit` anpassen (oder `tools/make-flathub-manifest.sh <tag>` neu ausführen) und als PR einreichen. Der Flathub-Bot (`x-checker-data`) schlägt neue Tags auch selbst als PR vor.
