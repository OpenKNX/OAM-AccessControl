# OAM-Fingerprint

OpenKNX Firmware und Anwendung zur OpenKNX-kompatiblen Einbindung von Fingerabdrucklesern.

## Kompatible Fingerabdruckleser

Aktuell werden folgende Fingerabdruckleser unterstützt:
- R503 (Speicherkapazität: 200 Finger)
- R503S (Speicherkapazität: 150 Finger)
- R503Pro (Speicherkapazität: 1500 Finger)

Weitere Fingerabdruckleser können bei Bedarf angebunden werden.

## Kompatible Hardware

### AB-Tools Fingerprint-Modul
Die kompakte Steuerplatine kann vollständig in ein Standard-Schalterprogramm integriert werden. Hier als Beispiel mit Gira-Schalterprogramm in matt schwarz und Edelstahlrahmen:

<kbd>![Gira vorne](hardware/fingerprint-with-gira-front.jpg)</kbd>

Bei Bedarf können zwei zusätzliche Touch-Buttons links/rechts unten im Gira-Rahmen verbaut werden, welche z. B. zum Schließen/Öffnen einer Tür verwendet werden können. In der Mitte zwischen den beiden Touch-Buttons kann hinter einer kleinen Öffnung noch eine rot/grüne LED zur Anzeige des Status verbaut werden.

Dabei ist lediglich eine Einbautiefe von ca. 2 cm in einer Standard-60-mm-Unterputzdose erforderlich:

<kbd>![Gira vorne](hardware/fingerprint-in-mounting-box.jpg)</kbd>

Alle Infos zur kompatiblen Hardware findet sich [hier](https://www.ab-smarthouse.com/produkt/openknx-fingerprint-leser/) oder bei Interesse einfach Kontakt per [E-Mail](mailto:info@ab-smarthouse.com?subject=[OpenKNX]%20Fingerprint) aufnehmen.

### SEN-UP1-8xTH
Ursprünglich als Sensormodul entwickelt, kann dieses Modul auch für den Fingerprint genutzt wwerden.
<kbd>![UP1 Pinout](hardware/PinoutSenUP1-8TH.png)</kbd>
Dabei wird allerdings kein Relais (Schaltaktorkanal) unterstützt.


https://muster.ing-dom.de/OpenKNX-UP1/OpenKNX-SEN-UP1-8xTH.html