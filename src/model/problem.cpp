#include "problem.h"

Problem neuesProblem(Schweregrad grad, const QString& code, const QString& datei,
                     int zeile, const QString& objektId, const QStringList& werte)
{
    Problem p;
    p.grad = grad;
    p.code = code;
    p.datei = datei;
    p.zeile = zeile;
    p.objektId = objektId;
    p.werte = werte;
    return p;
}

QString Problem::gradText() const
{
    if (grad == Schweregrad::Fehler) return tr("Fehler");
    if (grad == Schweregrad::Warnung) return tr("Warnung");
    return tr("Info");
}

QString Problem::text() const
{
    // Fehlende Werte durch Leerstrings ersetzen, damit arg() immer funktioniert
    QStringList w = werte;
    while (w.size() < 4) w.append(QString());

    // CSV-Dateien allgemein
    if (code == "DATEI_OEFFNEN")
        return tr("Datei konnte nicht geöffnet werden: %1").arg(w[0]);
    if (code == "SPALTEN_ANZAHL")
        return tr("Zeile hat %1 statt %2 Spalten und wurde übersprungen").arg(w[0], w[1]);
    if (code == "MASTER_FEHLT")
        return tr("Der Master-Index fehlt; es werden die Indexdateien der Batch-Ordner verwendet");
    if (code == "INDEX_FEHLT")
        return tr("Im Index ist kein Protokoll für Anlage %1 am %2 eingetragen").arg(w[0], w[1]);
    if (code == "SPALTE_REPARIERT")
        return tr("Zeile hatte %1 statt %2 Spalten; die fehlende Spalte '%3' wurde mit 0 ergänzt").arg(w[0], w[1], w[2]);
    if (code == "ID_DOPPELT")
        return tr("Doppelte ID '%1': der zweite Eintrag wird ignoriert").arg(w[0]);
    if (code == "WERT_LEER")
        return tr("Pflichtfeld '%1' ist leer").arg(w[0]);
    if (code == "ZAHL_UNGUELTIG")
        return tr("Feld '%1' enthält keine gültige Zahl ('%2')").arg(w[0], w[1]);
    if (code == "DEZIMALKOMMA")
        return tr("Feld '%1' verwendet ein Dezimalkomma ('%2') und wurde umgewandelt").arg(w[0], w[1]);
    if (code == "DATUM_UNGUELTIG")
        return tr("Ungültiges Datum '%1'").arg(w[0]);

    // Verweise zwischen Dateien
    if (code == "FK_LINIE")
        return tr("Verweist auf die unbekannte Linie '%1'").arg(w[0]);
    if (code == "FK_ANLAGE")
        return tr("Verweist auf die unbekannte Anlage '%1'").arg(w[0]);

    // Linien
    if (code == "SCHICHTMODELL_UNBEKANNT")
        return tr("Unbekanntes Schichtmodell '%1'").arg(w[0]);

    // Anlagen
    if (code == "TAKT_UNGUELTIG")
        return tr("Soll-Taktzeit muss größer als 0 sein (gelesen: '%1')").arg(w[0]);
    if (code == "TYP_UNBEKANNT")
        return tr("Unbekannter Anlagentyp '%1'").arg(w[0]);
    if (code == "STATUS_UNBEKANNT")
        return tr("Unbekannter Status '%1'").arg(w[0]);
    if (code == "BAUJAHR_UNPLAUSIBEL")
        return tr("Baujahr %1 ist unplausibel (erlaubt: %2 bis %3)").arg(w[0], w[1], w[2]);
    if (code == "BEZEICHNUNG_LINIE")
        return tr("Bezeichnung '%1' passt nicht zur Linie '%2'").arg(w[0], w[1]);
    if (code == "POSITION_DOPPELT")
        return tr("Position %1 ist in Linie %2 mehrfach vergeben").arg(w[0], w[1]);
    if (code == "WERT_NEGATIV")
        return tr("Feld '%1' darf nicht negativ sein (gelesen: '%2')").arg(w[0], w[1]);

    // Stoercodes und Wartungen
    if (code == "KATEGORIE_UNBEKANNT")
        return tr("Unbekannte Störcode-Kategorie '%1'").arg(w[0]);
    if (code == "DAUER_UNGUELTIG")
        return tr("Dauer muss größer als 0 sein (gelesen: '%1')").arg(w[0]);
    if (code == "GEPLANT_UNGUELTIG")
        return tr("Feld 'geplant' muss 'ja' oder 'nein' sein (gelesen: '%1')").arg(w[0]);

    // Protokoll-Index und XML-Dateien
    if (code == "INDEX_DOPPELT")
        return tr("Der Index enthält Anlage %1 am %2 mehrfach").arg(w[0], w[1]);
    if (code == "DATEI_FEHLT")
        return tr("Protokolldatei fehlt: %1").arg(w[0]);
    if (code == "XML_SYNTAX")
        return tr("XML-Syntaxfehler (Zeile %1, Spalte %2): %3").arg(w[0], w[1], w[2]);
    if (code == "XML_KOPF")
        return tr("Kopfdaten der Datei (%1, %2) passen nicht zum Index (%3, %4)").arg(w[0], w[1], w[2], w[3]);
    if (code == "XML_ANZAHL")
        return tr("Der Index nennt %1 Zustände, die Datei enthält %2").arg(w[0], w[1]);
    if (code == "SCHICHT_UNBEKANNT")
        return tr("Unbekannte Schicht '%1'").arg(w[0]);
    if (code == "SCHICHT_ANZAHL")
        return tr("Das Protokoll enthält %1 statt 3 Schichten").arg(w[0]);
    if (code == "UNGEPLANT_MIT_ZUSTAND")
        return tr("Die nicht geplante Schicht '%1' enthält Zustände").arg(w[0]);
    if (code == "GEPLANT_OHNE_ZUSTAND")
        return tr("Die geplante Schicht '%1' enthält keine Zustände").arg(w[0]);
    if (code == "ZUSTAND_TYP")
        return tr("Unbekannter Zustandstyp '%1' um %2").arg(w[0], w[1]);
    if (code == "ZEIT_RUECKWAERTS")
        return tr("Ungültiges Zeitintervall: Beginn %1 liegt nicht vor dem Ende %2").arg(w[0], w[1]);
    if (code == "ZEIT_LUECKE")
        return tr("Lücke zwischen %1 und %2 in Schicht '%3'").arg(w[0], w[1], w[2]);
    if (code == "ZEIT_UEBERLAPP")
        return tr("Überlappung ab %1 in Schicht '%2'").arg(w[0], w[1]);
    if (code == "ZEIT_AUSSERHALB")
        return tr("Zustand von %1 bis %2 liegt außerhalb der Schicht '%3'").arg(w[0], w[1], w[2]);
    if (code == "STUECK_NEGATIV")
        return tr("Negative Stückzahl (%1 = %2) um %3; Wert wurde auf 0 gesetzt").arg(w[0], w[1], w[2]);
    if (code == "STUECK_UNGUELTIG")
        return tr("Stückzahl '%1' ist nicht lesbar (%2 um %3)").arg(w[0], w[1], w[2]);
    if (code == "CODE_UNBEKANNT")
        return tr("Unbekannter Störcode '%1' um %2").arg(w[0], w[1]);
    if (code == "CODE_FEHLT")
        return tr("Störung ohne Störcode um %1").arg(w[0]);
    if (code == "PRODUKT_UNBEKANNT")
        return tr("Unbekannte Produkt-ID '%1' um %2").arg(w[0], w[1]);
    if (code == "PRODUKT_FEHLT")
        return tr("Produktionszustand ohne Produkt um %1").arg(w[0]);
    if (code == "PRODUKT_LINIE")
        return tr("Produkt '%1' gehört zur Linie '%2', die Anlage steht aber in Linie '%3'").arg(w[0], w[1], w[2]);

    return tr("Unbekanntes Problem (%1)").arg(code);
}
