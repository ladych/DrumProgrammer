# Latenzmessung (Q-01, Q-02)

Ziel nach Q-01: Buffer 128 Samples bei 48 kHz läuft ohne Dropouts, Anschlag bis Ton ≤ 6 ms (Ziel 5 ms).

## Was die Statuszeile anzeigt

Die Statuszeile unten im Hauptfenster zeigt Treiber, Gerät, Samplerate, Buffer und zwei Werte:

- **Latenz**: die Ausgangslatenz, die der Treiber meldet (bei JACK die Port-Latenz, also alle Perioden). Meldet der Treiber nichts, steht dort ein Buffer.
- **Anschlag bis Ton max.**: Ausgangslatenz plus ein Buffer. Ein Anschlag wartet höchstens einen Buffer, bis der nächste Audio-Block ihn abholt.

Das sind berechnete Werte. Die echte Latenz misst man so:

## 1. Ausgangs- und Round-Trip-Latenz mit jack_iodelay

1. Ausgang des Interfaces per Kabel auf einen Eingang legen (Loopback), z. B. Scarlett Out 1 → In 1. Pegel niedrig halten.
2. JACK mit 48 kHz, 128 Frames/Periode, 2 Perioden starten, z. B. `jackd -d alsa -d hw:USB -r 48000 -p 128 -n 2`.
3. `jack_iodelay` starten und verbinden: `jack_connect jack_delay:out system:playback_1` und `jack_connect system:capture_1 jack_delay:in`.
4. `jack_iodelay` gibt die gemessene Round-Trip-Latenz in Frames und ms aus, abzüglich der Systemlatenz die „extra loopback latency“.

Die Ausgangslatenz ist ungefähr die Hälfte der Round-Trip-Latenz. Sie sollte mit dem Wert „Latenz“ in der Statuszeile übereinstimmen.

## 2. Anschlag bis Ton

1. Drum Programmer starten, unter Audio → Einstellungen JACK, Buffer 128, das MIDI-Drumkit als Eingang wählen.
2. Ein Sample auf die Snare (38) laden.
3. Mikrofon direkt an das Pad halten und gleichzeitig den Kopfhörer- oder Line-Ausgang aufnehmen (zwei Spuren, z. B. in Reaper oder Audacity mit einem zweiten Interface-Eingang).
4. Mehrmals auf das Pad schlagen. Abstand zwischen Schlaggeräusch (Mikrofon) und Sample-Beginn (Ausgang) in der Aufnahme messen.

Der Wert enthält zusätzlich die Latenz des Drum-Moduls (MIDI-Ausgabe, typisch 1–3 ms über USB). Für die Computertastatur gilt dasselbe Verfahren mit dem Tastenklick als Referenz; dort kommen Tastatur-Abfrage und X11 hinzu, deshalb ist das MIDI-Drumkit die Referenz-Eingabe.

## 3. Dropouts

Bei Buffer 128 einige Minuten spielen und auf Knackser achten. JACK zählt XRuns (`jack_xrun` im Log oder in QjackCtl). Ab AP11 gibt es den 30-Minuten-Dauertest (Q-03).
