<!-- 
cSpell:words knxprod EEPROM Ausgangstrigger Sonnenstandsbezogene Sonnenauf vollzumüllen Enocean Pieptönen platformio
cSpell:words softwareseitig untergangszeit Urlaubsinfo Feiertagsinfo Konverterfunktionen Vergleicher Geokoordinaten
cSpell:words Konstantenbelegung vorzubelegen Intervallvergleich Hysteresevergleich Uebersicht Logiktrigger priorität
cSpell:words Szenenkonverter Szenennummern Zahlenbasierte Intervallgrenzen Hystereseschalter Ganzzahlbasierte
cSpell:words erwartungskonform hardwareabhängig Rueckkopplung eingabebereit maliges AUSschaltverzögerung EINschaltverzögerung
cSpell:words Triggersignal expample runterladen Wiregateway updatefähige Updatefunktion Auskühlalarm Zaehler tagestrigger
cSpell:words mgeramb ambiente Ambientenbeleuchtung
-->

# Applikationsbeschreibung Zugangskontrolle

Die Applikation Fingerprint erlaubt eine Parametrisierung einer Zutrittskontrolle per Fingerabdruck oder NFC-Tag mit der ETS.

Es gibt eine kleinere Applikationsversion mit bis zu 200 Aktionen/Fingerzuordnungen/NFC-Tags und eine größere mit bis zu 1500 Aktionen/Fingerzuordnungen/NFC-Tags. Während die größere hauptsächlich für das "großen" Lesegerät R503Pro angeboten wird, kann sie bei Bedarf an mehr als 200 Fingerzuordnungen auch für das "kleinere" Lesegerät R503 verwendet werden.

## Änderungshistorie

Im folgenden werden Änderungen an dem Dokument erfasst, damit man nicht immer das Gesamtdokument lesen muss, um Neuerungen zu erfahren.


26.02.2025: Firmware 0.7, Applikation 0.7

* NEU: Die Applikation wurde in "Zugangskontrolle" umbenannt.
* NEU: Die Abfrage von NFC-Tags wird jetzt unterstützt.

10.07.2024: Firmware 0.3, Applikation 0.3

* NEU: Man kann jetzt einstellen, ob der Finger nur bei einem Touch oder fortlaufend abgefragt wird.
* NEU: Es gibt jetzt ein KO, dass signalisiert, dass der Hardware-Scanner von der Hauptplatine getrennt wurde.
* NEU: Für Aktionen vom Typ "Umschalten" wird nach einem Neustart das Status-KO vom Bus gelesen.
* NEU: Das Schaltaktor-Modul wurde hinzugefügt.
* NEU: Das Binäreingang-Modul wurde hinzugefügt.
* NEU: Das Konfigurationstransfer-Modul wurde hinzugefügt.
* NEU: Das Logikmodul wurde auf die Version 3.3 aktualisiert.

02.06.2024: Firmware 0.2.1, Applikation 0.2

* KO "Scan: Erfolg" liefert nun eine "0", wenn ein Finger nicht erkannt wurde.

01.06.2024: Firmware 0.2.0, Applikation 0.2

* Es gibt nun eine Personensuche, mit der man nach den zugewiesenen Fingern und Namen suchen kann.
* Die Synchronisation zwischen mehreren Fingerabdrucklesern ist nun vollständig implementiert. Ist diese aktiviert, erfolgt die Synchronisation automatisch, nachdem ein neuer Finger angelernt wurde. Über die ETS ist es aber auch möglich die Synchronisation einzelner Finger manuell anzustoßen. Auch das Löschen eines Fingers wird synchronisiert.
* Die Funktion und Farbe des Fingerprint LED-Rings kann nun über Rohdaten-KOs auch extern gesteuert werden.

18.05.2024: Firmware 0.1.0, Applikation 0.1

* Initiales Release als OpenKNX Fingerprint

## **Verwendete Module**

Die Zugangskontrolle verwendet weitere OpenKNX-Module, die alle ihre eigene Dokumentation besitzen. Im folgenden werden die Module und die Verweise auf deren Dokumentation aufgelistet.

### **OpenKNX**

Dies ist eine Seite mit allgemeinen Parametern, die unter [Applikationsbeschreibung-Common](https://github.com/OpenKNX/OGM-Common/blob/v1/doc/Applikationsbeschreibung-Common.md) beschrieben sind. 

### **Konfigurationstransfer**

Der Konfigurationstransfer erlaubt einen

* Export von Konfigurationen von OpenKNX-Modulen und deren Kanälen
* Import von Konfigurationen von OpenKNX-Modulen und deren Kanälen
* Kopieren der Konfiguration von einem OpenKNX-Modulkanal auf einen anderen
* Zurücksetzen der Konfiguration eines OpenKNX-Modulkanals auf Standardwerte

Die Funktionen vom Konfigurationstranfer-Modul sind unter [Applikationsbeschreibung-ConfigTransfer](https://github.com/OpenKNX/OFM-ConfigTransfer/blob/v1/doc/Applikationsbeschreibung-ConfigTransfer.md) beschrieben.

### **Schaltaktor**

--ToDo--



### **Logiken**

Wie die meisten OpenKNX-Applikationen enthält auch diese Applikation ein Logikmodul.

Die Funktionen des Logikmoduls sind unter [Applikationsbeschreibung-Logik](https://github.com/OpenKNX/OFM-LogicModule/blob/v1/doc/Applikationsbeschreibung-Logik.md) beschrieben.

### **Virtuelle Taster**

Es werden auch virtuelle Taster von der Applikation angeboten. Mit der Nutzung der Binäreingänge oder der Touch-Platine (als Erweiterung) - können es auch echte Taster werden.

Die Funktionen des Tastermoduls sind unter [Applikationsbeschreibung-Taster](https://github.com/OpenKNX/OFM-VirtualButton/blob/v1/doc/Applikationsbeschreibung-Taster.md) beschrieben.

### **Binäreingänge**

Diese Applikation unterstützt auch Binäreingänge.

Die Funktionen der Binäreingänge sind unter [Applikationsbeschreibung-Binäreingang](https://github.com/OpenKNX/OFM-BinaryInput/blob/v1/doc/Applikationsbeschreibung-Binaereingang.md) beschrieben.



# **Zugangskontrolle**

<!-- DOC HelpContext="Dokumentation" -->
Mit diesem Modul können Finger und NFC-Tags im Lesegerät angelernt, gelöscht, Aktionen verknüpft und Finger bzw. NFC-Tags den Aktionen zugeordnet werden.

<!-- DOCCONTENT 
https://github.com/OpenKNX/OAM-AccessControl/blob/main/doc/Applikationsbeschreibung-AccessControl.md
DOCCONTENT -->

## **Allgemein**

In der Titelzeile wird der Modulname und dessen Version ausgegeben. Diese Information ist für Support-Anfragen im OpenKNX-Forum relevant.

### **Hardware**

In diesem Bereich erfolgen Einstellungen, die die Hardware-Scanner für Zugangskontrolle betreffen.

Detaileinstellungen erfolgen dann auf passenden Unterseiten, benannt nach der jeweiligen Hardware.

<!-- DOC -->
#### **Fingerprint Scanner**

Auswahl der angeschlossenen Fingerprint-Scanner-Hardware. Es wird die folgendes angeboten:

* **Kein Fingerprint**: Wenn keine Fingerprint-Hardware angeschlossen ist
* **R503** (Standard): Speicherplatz für 200 Finger
* **R503S**: Speicherplatz für 150 Finger
* **R503Pro**: Speicherplatz für 1500 Finger

<!-- DOC -->
#### **NFC Scanner**

Auswahl der angeschlossenen NFC-Scanner-Hardware. Es wird die folgendes angeboten:

* **Kein NFC** (Standard): Wenn keine NFC-Scanner-Hardware angeschlossen ist
* **NFC auf der Touch-Frontplatine**: Die Touch-Frontplatine ist mit der NFC-Scanner-Hardware ausgerüstet, zu erkennen an der NFC-Antenne

<!-- DOC -->
#### **Touch-Frontplatine vorhanden**

<!-- DOC Skip="1" -->
Erscheint nur, wenn bei "NFC Scanner" der Wert "Kein NFC" gewählt wurde.

Mit der Touch-Frontplatine werden 2 Touch-Sensortasten in der Applikation verfügbar gemacht.

### Zusatzfunktionen

<!-- DOC -->
#### **Rohdaten auf den Bus senden**

Die Hardware-Scanner können ihre Daten direkt auf den Bus senden, ohne jegliche Aktionszuordnungen "dazwischen".

Bei Aktivierung werden entsprechende Kommunikationsobjekte freigeschaltet.

<!-- DOC -->
#### **Zutrittsdaten-KOs aktivieren**

<!-- DOC Skip="1" -->
Erscheint nur, wenn bei "Rohdaten auf den Bus senden" ein "Ja" gewählt wurde.

Werden die speziellen Kommunikationsobjekte für Zutrittsdaten benötigt (DPT 15), können diese hier aktiviert werden.

<!-- DOC -->
#### **Synchronisation mehrerer Geräte**

Sind mehrere OpenKNX-Zugangskontroll-Geräte vorhanden und sollen die Fingerprint- und NFC-Daten unter diesen Geräten synchronisiert werden, muss diese Option aktiviert werden.

Es stehen daraufhin zusätzliche Kommunikationsobjekte zur Synchronisation zur Verfügung.

<!-- DOC -->
#### **Verzögerung zwischen Sync-Telegrammen**

<!-- DOC Skip="1" -->
Erscheint nur, wenn bei "Synchronisation mehrerer Geräte" ein "Ja" gewählt wurde.

Um eine zu hohe Busbelastung zu vermeiden, wird hier die Verzögerung zwischen Sync-Telegrammen in Millisekunden festgelegt.

<!-- DOC -->
#### **Externe Kontrolle ermöglichen**

Bei Aktivierung werden entsprechende Kommunikationsobjekte freigeschaltet, die dazu verwendet werden können den Scanner extern zu steuern (um z.B. einen Anlernvorgang extern anzustoßen).

## **Fingerprint-Scanner**

Erscheint nur, wenn bei Fingerprint-Scanner eine entscprechende Hardware ausgewählt wurde.
Erscheint als Unterseite der Seite "Allgemein".

### **Hardware-Einstellungen**

Hier werden Detaileinstellungen zur Fingerprint-Scanner-Hardware vorgenommen.

<!-- DOC -->
#### **Fingerabfrage**

Normalerweise wird die Fingerabfrage "Bei Berührung" des Fingerprints gestartet. Es gib aber einige Fälle, in den der Touch nicht zuverlässig erkannt wird. In solchen Fällen kann mit der Einstellung "Fortlaufend" die Fingerabfrage unabhängig von einer Berührung erfolgen.

Die Einstellung "Fortlaufend" kann zur Folge haben, dass Logiken, die auf dem Fingerprint definiert sind, nicht mehr zuverlässig bzw. stark verzögert laufen. Sie sollte nur gewählt werden, wenn die normale Erkennung "Bei Berührung" nicht funktioniert.


### **Finger bearbeiten**

Dieser Bereich stellt einen Finger-Editor dar, mit dem man in der ETS einzelne Finger anlernen, ändern oder löschen kann.

Dieser Bereich kann erst funktionieren, wenn das Gerät das erste Mal programmiert worden ist, also eine PA und eine Applikation hat. Solange die entsprechenden Buttons ausgegraut sind, sind diese Voraussetzungen nicht erfüllt.

<!-- DOC -->
#### **Finger**

Dieses Auswahlfeld wählt die gewünschte Editierfunktion aus:

* **anlernen**: Zu einer neuen Finger-ID soll ein Finger einer Person angelernt werden
* **ändern**: Zu einer bestimmten Finger-ID soll der Finger oder die Person geändert werden
* **löschen**: Eine bestimmte Finger-ID und die damit verbundenen Finger- und Personendaten werden gelöscht.

### **Finger anlernen**

Zum Anlernen muss eine neue Finger-ID gewählt werden, dann der Name und der Finger der Person eingegeben werden und anschließend der Button "Anlernen" gedrückt werden.

#### **Name der Person**

Der Name der Person, welcher zusammen mit den neu angelernten Fingerdaten gespeichert werden soll.

#### **Finger der Person**

Der Finger der Person, welcher zusammen mit den neu angelernten Fingerdaten gespeichert werden soll.

#### **Scanner Finger ID**

Die ID des Fingers (= der Speicherplatz), auf welche die neu angelernten Fingerdaten gespeichert werden soll.

Dabei sind die verfügbaren SPeicherplätze abhängig von der ausgewählten Hardware des Fingerprint-Scanners:
Sie werden dabei von 0 beginnend durchnummeriert. Hat der Scanner also beispielsweise 200 Speicherplätze, stehen die IDs 0-199 zur Verfügung.

### **Finger ändern**

Zum Ändern muss eine existierende Finger-ID gewählt werden, dann der neue Name und der neue Finger der Person eingegeben werden und anschließend der Button "Ändern" gedrückt werden.

#### **Name der Person**

Der Name der Person, welche neu den existierenden Fingerdaten zugeordnet werden soll.

#### **Finger der Person**

Der Finger der Person, welcher neu den existierenden Fingerdaten zugeordnet werden soll.

#### **Scanner Finger ID**

Die existierende ID des Fingers (= der Speicherplatz), dem die neue Persond und/oder der neue Finger zugeordnet werden soll.

### **Finger löschen**

Zum Löschen muss eine existierende (vorher angelernte) Finger-ID eingegeben werden. Die Person, der Finger und die angelernten Fingerdaten werden gelöscht, sobald der Button "Löschen" gedrückt wurde.

#### **Scanner Finger ID**

Die ID des Fingers (= der Speicherplatz), die gelöscht werden soll.

<!-- DOC HelpContext="Name der Person" -->
<!-- DOCCONTENT
Der Name der Person, die einer bestimmten Finger-ID zugeordnet ist.
DOCCONTENT -->

<!-- DOC HelpContext="Finger der Person" -->
<!-- DOCCONTENT
Der Finger der Person, der einer bestimmten Finger-ID zugeordnet ist.
DOCCONTENT -->

<!-- DOC HelpContext="Scanner Finger ID" -->
<!-- DOCCONTENT
Die ID des Fingers (= der Speicherplatz), auf welche die Fingerdaten gespeichert sind.

Dabei sind die verfügbaren Speicherplätze abhängig von der ausgewählten Hardware des Fingerprint-Scanners. 
Sie werden dabei von 0 beginnend durchnummeriert. Hat der Scanner also beispielsweise 200 Speicherplätze, stehen die IDs 0-199 zur Verfügung.
DOCCONTENT -->

### **Finger synchronisieren**

Nach jedem Anlernvorgang wird der neue Finger mit den anderen Geräten automatisch synchronisiert. Da dieser Vorgang komplett asynchron passiert, gibt es keine Garantie und keine Rückmeldung, ob die Synchronisation funktioniert hat. 

Falls auf irgendeinem Gerät eine Finger ID fehlt oder nicht aktuell ist, kann man hier diese Finger ID erneut synchronisieren lassen.

<!-- DOC -->
#### **Finger ID synchronisieren**

Durch die Eingabe einer Finger ID und das Drücken des Buttons "Synchronisieren" kann hier die Synchronisation eines bestimmten Fingers manuell angestoßen werden.

### **Gefährliche Funktionen**

Die Funktionen in diesem Bereich können zum Datenverlust führen, der nicht behebbar ist. Bitte mit Vorsicht nutzen.

<!-- DOC -->
#### **Passwort**

Wenn Sie hier ein Passwort vergeben und dieses vergessen, wird das Fingerprint-Lesegerät unbrauchbar. Das Passwort kann nicht wiederhergestellt werden!

Sie haben die Möglichkeit ein Passwort erstmalig festzulegen oder ein bereits vorhandenes zu ändern. Im letzteren Fall muss auch das alte Passwort eingegeben werden.

<!-- DOC -->
##### **Altes Passwort**

Das bereits vorhandene Passwort, welches geändert werden soll.

<!-- DOC -->
##### **Neues Passwort**

Das neue, zu setzende Passwort.

<!-- DOC -->
#### **Alle Finger löschen?**

Mit dieser Funktion werden sämtliche gespeicherten Fingerdaten inklusive Personenzuordnung unwiderruflich gelöscht.





## **NFC-Scanner**

Erscheint nur, wenn bei NFC-Scanner eine entscprechende Hardware ausgewählt wurde.
Erscheint als Unterseite der Seite "Allgemein".


### **NFC-Tag bearbeiten**

Dieser Bereich stellt einen NFC-Tag-Editor dar, mit dem man in der ETS einzelne NFC-Tags anlernen, anlegen, ändern oder löschen kann.

Dieser Bereich kann erst funktionieren, wenn das Gerät das erste Mal programmiert worden ist, also eine PA und eine Applikation hat. Solange die entsprechenden Buttons ausgegraut sind, sind diese Voraussetzungen nicht erfüllt.

<!-- DOC -->
#### **NFC-Tag**

Dieses Auswahlfeld wählt die gewünschte Editierfunktion aus:

* **anlernen**: Zu einer neuen Tag ID soll ein neuer NFC-Tag angelernt werden
* **anlegen**: Zu einer neuen Tag ID soll ein neuer NFC-Tag angelegt werden, dessen Tag Schlüssel (UID) man kennt.
* **ändern**: Zu einer bestimmten Tag ID soll der NFC-Tag geändert werden
* **löschen**: Eine bestimmte Tag ID und die damit verbundenen Tagdaten werden gelöscht.

### **NFC-Tag anlernen**

Zum Anlernen muss eine neue Tag ID gewählt werden, dann der Name des Tags eingegeben werden und anschließend der Button "Anlernen" gedrückt werden.

#### **Tag Name**

Der Name des NFC-Tags, der angelernt werden soll.

#### **Tag ID**

Die ID des NFC-Tags (= der Speicherplatz), auf welchen die neuen Daten gespeichert werden sollen.

#### **Tag Schlüssel (UID)**

In diesem Feld erscheint der Tag Schlüssel (auch bekannt als UID) nach einem erfolgreichen Anlernvorgang.

### **NFC-Tag anlegen**

Zum Anlegen muss eine neue Tag ID gewählt werden, dann der Name des Tags eingegeben werden und der Tag Schlüssel (UID). Anschließend der Button "Anlegen" gedrückt werden.

#### **Tag Name**

Der Name des NFC-Tags, der angelegt werden soll.

#### **Tag ID**

Die ID des NFC-Tags (= der Speicherplatz), auf welchen die neuen Daten gespeichert werden sollen.

#### **Tag Schlüssel (UID)**

Der Tag Schlüssel (auch bekannt als UID) des neuen NFC-Tags.


### **NFC-Tag ändern**

Zum Ändern muss eine existierende Tag ID gewählt werden, dann der neue Name des NFC-Tags und/oder der Tag Schlüssel eingegeben werden und anschließend der Button "Ändern" gedrückt werden.

#### **Tag Name**

Der Name des NFC-Tags, der geändert werden soll.

#### **Tag ID**

Die ID des NFC-Tags (= der Speicherplatz), auf welchem die geänderten Daten gespeichert werden sollen.

#### **Tag Schlüssel (UID)**

Der Tag Schlüssel (auch bekannt als UID) des NFC-Tags.


### **NFC-Tag löschen**

Zum löschen muss eine existierende (vorher angelernte) Tag ID eingegeben werden. Der NFC-Tag und der Tag Schlüssel werden gelöscht, sobald der Button "löschen" gedrückt wurde.

<!-- DOC HelpContext="Tag Name" -->
<!-- DOCCONTENT
Der Name des NFC-Tags, der einer bestimmten Tag ID zugeordnet ist.
DOCCONTENT -->

<!-- DOC HelpContext="Tag ID" -->
<!-- DOCCONTENT
Die ID des Tags (= der Speicherplatz), auf welchem die NFC Tag Daten gespeichert sind.
DOCCONTENT -->

<!-- DOC HelpContext="Tag Schlüssel (UID)" -->
<!-- DOCCONTENT
Der Schlüssel des NFC-Tags, auch bekannt als UID, der einer bestimmten Tag ID zugeordnet ist.
DOCCONTENT -->


### **NFC-Tag synchronisieren**

Nach jedem Anlernvorgang wird der neue NFC-Tag mit den anderen Geräten automatisch synchronisiert. Da dieser Vorgang komplett asynchron passiert, gibt es keine Garantie und keine Rückmeldung, ob die Synchronisation funktioniert hat. 

Falls auf irgendeinem Gerät eine Tag ID fehlt oder nicht aktuell ist, kann man hier diese Tag ID erneut synchronisieren lassen.

<!-- DOC -->
#### **Tag ID synchronisieren**

Durch die Eingabe einer Tag ID und das Drücken des Buttons "Synchronisieren" kann hier die Synchronisation eines bestimmten NFC-Tags manuell angestoßen werden.

### **Gefährliche Funktionen**

Die Funktionen in diesem Bereich können zum Datenverlust führen, der nicht behebbar ist. Bitte mit Vorsicht nutzen.

<!-- DOC -->
#### **Alle NFC-Tags löschen?**

Mit dieser Funktion werden sämtliche gespeicherten NFC-Tags inklusive aller Tag Namen und Tag Schlüssel unwiderruflich gelöscht.



<!-- DOC -->
## Aktionen

Aktioen sind die Objekte, die beim Zugangssystem etwas machen. Aktionen können benannt werden und definieren das Ausgangs-KO und dessen verhalten, wenn diese Aktion ausgelöst wird.

<!-- DOC -->
### **Verfügbare Aktionen**

Hier kann hier ausgewählt werden, wie viele Aktionen sichtbar und editierbar sind. 

Die ETS ist auch schneller in der Anzeige, wenn sie weniger (leere) Aktionen darstellen muss. Insofern macht es Sinn, nur so viele Aktionen anzuzeigen, wie man wirklich braucht.

### Autorisierung

Autorisierungsaktionen werden nicht direkt vom Zugangssystem aufgerufen. Sie werden durch ein KO aufgerufen und durch das Zugangssystem autorisiert. Hier wird also das Zutrittssystem nicht dazu genutzt, die Aktion auszulösen sondern zu bestätigen, dass die Person diese Aktion ausführen darf, dafür also autorisiert ist. 

Auf diese Weise können mit einem Finger oder einem NFC-Tag viele Aktionen autorisiert werden.

<!-- DOC -->
#### **Warten auf Autorisierung**

Ist eine Autorisierung für eine Aktion angefordert, wird die hier angegebene Zeit auf das Auflegen eines Fingers oder das Vorhalten eines NFC-Tags am Scanner gewartet. Falls die Zeit ohne Autorisierung verstreicht, wird die Aktion abgebrochen.






## **Unterstützte Hardware**

Die Software für dieses Release wurde auf folgender Hardware getestet und läuft damit "out-of-the-box":

* **AB-SmartHouse Fingerprint-Leser** [www.ab-smarthouse.com](https://www.ab-smarthouse.com/produkt/openknx-fingerprint-leser/) als Basisplatine mit Finger-Lesegeräte R503, R503S und R503Pro

Andere Hardware kann genutzt werden, jedoch muss das Projekt dann neu kompiliert und gegebenenfalls angepasst werden. Alle notwendigen Teile für ein Aufsetzen der Build-Umgebung inklusive aller notwendigen Projekte finden sich im [OpenKNX-Projekt](https://github.com/OpenKNX).

Interessierte sollten auch die Beiträge im [OpenKNX-Forum](https://knx-user-forum.de/forum/projektforen/openknx) studieren.
