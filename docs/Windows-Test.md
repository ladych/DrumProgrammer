# Test auf dem Windows-Rechner (AP10)

Prüft F-AO-03 (ASIO, WASAPI als Fallback), Q-09 (Gerätewechsel, Interface abziehen) und Q-03 (Dauertest ohne Aussetzer). Ergebnis bitte im Pull Request oder in Issue #11 festhalten.

## Vorbereitung

1. Das Programm kommt aus der CI: im Lauf des Pull Requests unter „Artifacts" `DrumProgrammer-Windows` laden und entpacken. Mit ASIO ist es nur gebaut, wenn das Repository-Secret `ASIO_SDK_URL` gesetzt ist (sonst steht im Windows-Job die Warnung „building without ASIO").
2. Treiber des Interfaces (z. B. Focusrite Scarlett) mit ASIO-Treiber installieren.
3. Für einen echten ersten Start die alten Einstellungen entfernen: `%APPDATA%\DrumProgrammer\audio-device.xml` löschen.

## Ablauf

| # | Schritt | Erwartung | Anforderung |
|---|---------|-----------|-------------|
| 1 | Erster Start mit angeschlossenem Interface | Statusleiste zeigt „ASIO \| \<Treibername\>"; Testton und Kit klingen | F-AO-03 |
| 2 | Einstellungen (Zahnrad): Buffer 128 bei 48 kHz | Statusleiste zeigt Buffer 128 (2,7 ms) und die Latenz | F-AO-01, Q-02 |
| 3 | Programm beenden und neu starten | Gleiches Gerät und gleicher Buffer wie in Schritt 2 | F-AO-04 |
| 4 | Während der Wiedergabe das Interface abziehen | Kein Absturz; nach wenigen Sekunden Ton über die Lautsprecher, Statusleiste beginnt mit „Ersatz für ASIO: …" | Q-09 |
| 5 | Interface wieder einstecken | Nach wenigen Sekunden wieder ASIO über das Interface, Hinweis verschwindet | Q-09 |
| 6 | Programm mit abgezogenem Interface starten, dann einstecken | Start über WASAPI mit Hinweis, nach dem Einstecken ASIO | F-AO-03, Q-09 |
| 7 | In den Einstellungen auf „Windows Audio" wechseln und zurück auf ASIO | Ton nach jedem Wechsel, kein Absturz | Q-09 |
| 8 | MIDI-Drumkit während der Wiedergabe abziehen und wieder einstecken | Kein Absturz; nach dem Einstecken triggert es wieder | Q-09 |
| 9 | Szenarien M1 bis M3 aus Kapitel 10 des Pflichtenhefts | Wie unter Linux | M4 |
| 10 | Dauertest: 30 Minuten Song mit Live-Eingabe bei Buffer 128 | Kein hörbarer Aussetzer; „XRuns: 0" in der Statusleiste (ASIO zählt sie, WASAPI zeigt keinen Zähler) | Q-03 |

Die Datei `%APPDATA%\DrumProgrammer\audio-device.xml` behält während eines Fallbacks das gewählte ASIO-Gerät; das kann nach Schritt 4 geprüft werden.
