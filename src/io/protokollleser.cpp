#include "protokollleser.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

static const char* ZEITFORMAT = "yyyy-MM-dd HH:mm:ss";

std::shared_ptr<Schichtprotokoll> Protokollleser::lesen(const QString& dateiPfad,
                                                        const PruefKontext& kontext)
{
    auto protokoll = std::make_shared<Schichtprotokoll>();
    protokoll->quellPfad = dateiPfad;
    const QString dateiName = QFileInfo(dateiPfad).fileName();

    QFile datei(dateiPfad);
    if (!datei.exists()) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "DATEI_FEHLT",
                                                dateiName, 0, kontext.objektId, {dateiName}));
        return protokoll;
    }
    if (!datei.open(QIODevice::ReadOnly | QIODevice::Text)) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "DATEI_OEFFNEN",
                                                dateiName, 0, kontext.objektId, {dateiName}));
        return protokoll;
    }

    QXmlStreamReader xml(&datei);
    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (!xml.isStartElement()) continue;

        const QXmlStreamAttributes attribute = xml.attributes();
        const QString name = xml.name().toString();

        if (name == "schichtprotokoll") {
            protokoll->anlageId = attribute.value("anlage_id").toString().trimmed();
            protokoll->datum = QDate::fromString(attribute.value("datum").toString().trimmed(), "yyyy-MM-dd");
        }
        else if (name == "schicht") {
            Schicht schicht;
            schicht.name = attribute.value("name").toString().trimmed();
            schicht.beginn = QDateTime::fromString(attribute.value("beginn").toString().trimmed(), ZEITFORMAT);
            schicht.ende = QDateTime::fromString(attribute.value("ende").toString().trimmed(), ZEITFORMAT);
            schicht.geplant = (attribute.value("geplant").toString().trimmed().toLower() == "ja");
            protokoll->schichten.append(schicht);
        }
        else if (name == "zustand" && !protokoll->schichten.isEmpty()) {
            ZustandEintrag z;
            z.typRoh = attribute.value("typ").toString().trimmed();
            z.typ = zustandTypVonText(z.typRoh);
            z.beginn = QDateTime::fromString(attribute.value("beginn").toString().trimmed(), ZEITFORMAT);
            z.ende = QDateTime::fromString(attribute.value("ende").toString().trimmed(), ZEITFORMAT);
            z.produktId = attribute.value("produkt").toString().trimmed();
            z.stoerCode = attribute.value("code").toString().trimmed();

            // Stueckzahlen lesen: negative oder unlesbare Werte werden auf 0 gesetzt und gemeldet
            const QString zeit = z.beginn.toString("HH:mm:ss");
            const QString felder[2] = {"gut", "ausschuss"};
            for (int i = 0; i < 2; i++) {
                if (!attribute.hasAttribute(felder[i])) continue;
                const QString text = attribute.value(felder[i]).toString().trimmed();
                bool ok = false;
                int wert = text.toInt(&ok);
                if (!ok) {
                    protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "STUECK_UNGUELTIG",
                        dateiName, 0, kontext.objektId, {text, felder[i], zeit}));
                    wert = 0;
                    z.plausibel = false;
                } else if (wert < 0) {
                    protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "STUECK_NEGATIV",
                        dateiName, 0, kontext.objektId, {felder[i], text, zeit}));
                    wert = 0;
                    z.plausibel = false;
                }
                if (i == 0) z.gut = wert;
                else z.ausschuss = wert;
            }
            protokoll->schichten.last().zustaende.append(z);
        }
    }

    if (xml.hasError()) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "XML_SYNTAX",
            dateiName, 0, kontext.objektId,
            {QString::number(xml.lineNumber()), QString::number(xml.columnNumber()), xml.errorString()}));
        protokoll->geladen = false;
        return protokoll;
    }

    // Inhaltliche Pruefung
    if (protokoll->schichten.size() != 3) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "SCHICHT_ANZAHL",
            dateiName, 0, kontext.objektId, {QString::number(protokoll->schichten.size())}));
    }
    for (const Schicht& schicht : protokoll->schichten) {
        pruefeSchicht(schicht, kontext, protokoll.get());
    }

    protokoll->geladen = true;
    return protokoll;
}

void Protokollleser::pruefeSchicht(const Schicht& schicht, const PruefKontext& kontext,
                                   Schichtprotokoll* protokoll)
{
    const QString dateiName = QFileInfo(protokoll->quellPfad).fileName();
    const QString id = kontext.objektId;

    if (schicht.name != "frueh" && schicht.name != "spaet" && schicht.name != "nacht") {
        protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "SCHICHT_UNBEKANNT",
                                                dateiName, 0, id, {schicht.name}));
    }
    if (!schicht.geplant && !schicht.zustaende.isEmpty()) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "UNGEPLANT_MIT_ZUSTAND",
                                                dateiName, 0, id, {schicht.name}));
    }
    if (schicht.geplant && schicht.zustaende.isEmpty()) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "GEPLANT_OHNE_ZUSTAND",
                                                dateiName, 0, id, {schicht.name}));
    }

    // Die Zustaende einer geplanten Schicht muessen lueckenlos aufeinander folgen
    QDateTime letztesEnde = schicht.beginn;
    for (const ZustandEintrag& z : schicht.zustaende) {
        const QString von = z.beginn.toString("HH:mm:ss");
        const QString bis = z.ende.toString("HH:mm:ss");

        if (z.typ == ZustandTyp::Unbekannt) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "ZUSTAND_TYP",
                                                    dateiName, 0, id, {z.typRoh, von}));
        }
        if (!z.beginn.isValid() || !z.ende.isValid() || z.beginn >= z.ende) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Fehler, "ZEIT_RUECKWAERTS",
                                                    dateiName, 0, id, {von, bis}));
            continue;   // fuer die Lueckenpruefung unbrauchbar
        }
        if (z.beginn < schicht.beginn || z.ende > schicht.ende) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "ZEIT_AUSSERHALB",
                                                    dateiName, 0, id, {von, bis, schicht.name}));
        }
        if (letztesEnde.isValid() && z.beginn > letztesEnde) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "ZEIT_LUECKE",
                dateiName, 0, id, {letztesEnde.toString("HH:mm:ss"), von, schicht.name}));
        } else if (letztesEnde.isValid() && z.beginn < letztesEnde) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "ZEIT_UEBERLAPP",
                                                    dateiName, 0, id, {von, schicht.name}));
        }
        letztesEnde = z.ende;

        if (z.typ == ZustandTyp::Produktion && z.produktId.isEmpty()) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "PRODUKT_FEHLT",
                                                    dateiName, 0, id, {von}));
        }
        if (!z.produktId.isEmpty() && !kontext.produkte.isEmpty()) {
            if (!kontext.produkte.contains(z.produktId)) {
                protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "PRODUKT_UNBEKANNT",
                                                        dateiName, 0, id, {z.produktId, von}));
            } else if (!kontext.anlageLinie.isEmpty()
                       && kontext.produktLinie.value(z.produktId) != kontext.anlageLinie) {
                protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "PRODUKT_LINIE",
                    dateiName, 0, id, {z.produktId, kontext.produktLinie.value(z.produktId), kontext.anlageLinie}));
            }
        }
        if (z.typ == ZustandTyp::Stoerung && z.stoerCode.isEmpty()) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "CODE_FEHLT",
                                                    dateiName, 0, id, {von}));
        }
        if (!z.stoerCode.isEmpty() && !kontext.stoercodes.isEmpty()
            && !kontext.stoercodes.contains(z.stoerCode)) {
            protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "CODE_UNBEKANNT",
                                                    dateiName, 0, id, {z.stoerCode, von}));
        }
    }
    if (schicht.geplant && !schicht.zustaende.isEmpty() && letztesEnde.isValid()
        && schicht.ende.isValid() && letztesEnde < schicht.ende) {
        protokoll->probleme.append(neuesProblem(Schweregrad::Warnung, "ZEIT_LUECKE",
            dateiName, 0, id, {letztesEnde.toString("HH:mm:ss"), schicht.ende.toString("HH:mm:ss"), schicht.name}));
    }
}
