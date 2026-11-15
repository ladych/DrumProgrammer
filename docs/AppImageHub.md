# AppImageHub

Das AppImage von Drum Programmer soll im Katalog [AppImageHub](https://appimage.github.io/) erscheinen (AP25). AppImageHub verteilt selbst nichts: Der Katalog verweist auf die GitHub-Releases, holt bei jeder Prüfung das AppImage des neuesten Releases und liest alle Angaben (Name, Icon, Beschreibung, Lizenz, Screenshot) aus dem AppImage selbst.

## Was das AppImage dafür mitbringt

`tools/make-appimage.sh` baut das AppImage so, wie AppImageHub es prüft:

| Prüfung | Umsetzung |
|---|---|
| Startmenü-Eintrag besteht `desktop-file-validate`, Icon als PNG (`.DirIcon`) | `packaging/linux/io.github.ladych.DrumProgrammer.desktop`, Icon 512×512 |
| AppStream-Metainfo besteht `appstreamcli validate-tree` | Die Metainfo liegt im AppImage als `usr/share/metainfo/io.github.ladych.DrumProgrammer.appdata.xml`, weil AppImageHub Zusammenfassung und Beschreibung nur aus so benannten Dateien übernimmt. Im Flatpak heißt sie weiter `….metainfo.xml`. |
| Läuft auf der ältesten noch unterstützten Ubuntu-LTS (zurzeit 22.04) | Gebaut auf `ubuntu-22.04`, braucht glibc 2.35 |
| Hauptfenster innerhalb von 30 Sekunden, ohne Netz, englisch bei C-Locale, hell ohne Dunkel-Einstellung des Systems | Sprache und Farbschema folgen dem System (AP15, AP16) |
| Update-Information | `gh-releases-zsync\|ladych\|DrumProgrammer\|latest\|DrumProgrammer-*x86_64.AppImage.zsync`. Damit aktualisieren AppImageUpdate, AppImageLauncher und Gear Lever das Programm aus dem neuesten Release und laden nur die geänderten Teile. Dafür muss jedes Release neben dem AppImage die Datei `DrumProgrammer-<version>-x86_64.AppImage.zsync` enthalten; der Workflow „Release" lädt sie mit hoch. |

Den Screenshot für die Katalogseite nimmt AppImageHub aus der Metainfo (`docs/MainWindow.png` im Branch `main`); zusätzlich macht der Test selbst ein Bildschirmfoto des Hauptfensters unter Xvfb.

## Einreichung

Das macht der Inhaber des GitHub-Kontos `ladych` selbst, weil der Pull Request öffentlich ist und unter seinem Namen läuft.

1. Ein Release mit AppImage und `.zsync`-Datei muss das neueste Release sein (ab `v1.0.2`; `v1.0.1` hat noch keine Update-Information, die Prüfung bestünde aber mit Hinweisen).
2. Die Datei über den Link <https://github.com/AppImage/appimage.github.io/new/master/data> anlegen. GitHub legt dabei automatisch einen Fork an.
   - Dateiname: `Drum_Programmer` (der Name aus dem Startmenü-Eintrag, Leerzeichen als `_`, ohne Endung)
   - Inhalt, genau eine Zeile: `https://github.com/ladych/DrumProgrammer`
3. „Propose new file" und dann „Create pull request". Der PR darf nur diese eine Datei enthalten. Als Titel passt `Add Drum Programmer`; in der Beschreibung kurz erwähnen, dass es die eigene Anwendung ist.
4. Der Test läuft sofort (GitHub Actions) und schreibt das Ergebnis als Kommentar in den PR: bei Erfolg mit Screenshot und dem Label `screenshot-ok`, sonst mit Fehler und Hinweis. Nach einem neuen Release startet der Kommentar `/retest` die Prüfung erneut.
5. Ein Betreuer von AppImageHub führt den PR zusammen; das kann einige Tage dauern. Danach erscheint die Seite `https://appimage.github.io/Drum_Programmer/`.

Neue Versionen brauchen keine weitere Einreichung: Der Katalog verweist auf das Repository und nimmt jeweils das neueste Release.

## Lokal prüfen

Die Schritte des Tests lassen sich mit einem ausgepackten AppImage nachvollziehen:

```sh
./DrumProgrammer-1.0.2-x86_64.AppImage --appimage-extract
desktop-file-validate squashfs-root/*.desktop
appstreamcli validate-tree squashfs-root
./DrumProgrammer-1.0.2-x86_64.AppImage --appimage-updateinformation
LANG=C ./DrumProgrammer-1.0.2-x86_64.AppImage   # Hauptfenster englisch und hell
```

Das vollständige Prüfskript ist `code/worker.sh` im Repository [AppImage/appimage.github.io](https://github.com/AppImage/appimage.github.io).
