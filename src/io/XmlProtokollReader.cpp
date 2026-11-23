#include "io/XmlProtokollReader.h"
#include "core/LanguageManager.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

namespace RoboCell {

std::shared_ptr<Schichtprotokoll> XmlProtokollReader::readFromFile(
    const QString& filePath,
    const QSet<QString>& knownProducts,
    const QSet<QString>& knownStoercodes)
{
    auto proto = std::make_shared<Schichtprotokoll>();
    proto->quellPfad = filePath;

    QFile file(filePath);
    if (!file.exists()) {
        proto->loadSuccess = false;
        proto->errorMessage = trBi(
            QString("Protokolldatei nicht gefunden: %1").arg(filePath),
            QString("Log file not found: %1").arg(filePath)
        );
        return proto;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        proto->loadSuccess = false;
        proto->errorMessage = trBi(
            QString("Protokolldatei konnte nicht geöffnet werden: %1 (%2)").arg(filePath, file.errorString()),
            QString("Could not open log file: %1 (%2)").arg(filePath, file.errorString())
        );
        return proto;
    }

    QXmlStreamReader xml(&file);
    Schicht* currentSchicht = nullptr;

    while (!xml.atEnd() && !xml.hasError()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            const QString elemName = xml.name().toString();

            if (elemName == "schichtprotokoll") {
                const auto attrs = xml.attributes();
                proto->anlageId = attrs.value("anlage_id").toString().trimmed();
                proto->datum = QDate::fromString(attrs.value("datum").toString().trimmed(), "yyyy-MM-dd");
            } else if (elemName == "schicht") {
                Schicht s;
                const auto attrs = xml.attributes();
                s.name = attrs.value("name").toString().trimmed();
                s.beginn = QDateTime::fromString(attrs.value("beginn").toString().trimmed(), "yyyy-MM-dd HH:mm:ss");
                s.ende = QDateTime::fromString(attrs.value("ende").toString().trimmed(), "yyyy-MM-dd HH:mm:ss");
                const QString geplantStr = attrs.value("geplant").toString().trimmed().toLower();
                s.geplant = (geplantStr == "ja" || geplantStr == "true" || geplantStr == "1");
                proto->schichten.append(s);
                currentSchicht = &proto->schichten.last();
            } else if (elemName == "zustand" && currentSchicht != nullptr) {
                ZustandEintrag z;
                const auto attrs = xml.attributes();
                z.rawTyp = attrs.value("typ").toString().trimmed();
                z.typ = parseZustandTyp(z.rawTyp);
                z.beginn = QDateTime::fromString(attrs.value("beginn").toString().trimmed(), "yyyy-MM-dd HH:mm:ss");
                z.ende = QDateTime::fromString(attrs.value("ende").toString().trimmed(), "yyyy-MM-dd HH:mm:ss");

                // Zeitintervall-Prüfung (z.B. A007_2026-09-21.xml hat beginn > ende)
                if (!z.beginn.isValid() || !z.ende.isValid() || z.beginn >= z.ende) {
                    z.isPlausible = false;
                    z.warningMessage = trBi(
                        QString("Ungültiges Zeitintervall in Schicht '%1': Beginn (%2) >= Ende (%3)")
                            .arg(currentSchicht->name, attrs.value("beginn").toString(), attrs.value("ende").toString()),
                        QString("Invalid time interval in shift '%1': Start (%2) >= End (%3)")
                            .arg(currentSchicht->name, attrs.value("beginn").toString(), attrs.value("ende").toString())
                    );
                    proto->warnings.append(z.warningMessage);
                }

                // Zustandstyp-Prüfung (z.B. A051_2026-09-18.xml hat typ="stillstand")
                if (z.typ == ZustandTyp::Unbekannt) {
                    z.isPlausible = false;
                    QString msg = trBi(
                        QString("Unbekannter Zustandstyp '%1' um %2 (wird als ungeplanter Stillstand gewertet)")
                            .arg(z.rawTyp, z.beginn.toString("HH:mm:ss")),
                        QString("Unknown state type '%1' at %2 (treated as unplanned downtime)")
                            .arg(z.rawTyp, z.beginn.toString("HH:mm:ss"))
                    );
                    if (z.warningMessage.isEmpty()) z.warningMessage = msg;
                    proto->warnings.append(msg);
                }

                if (attrs.hasAttribute("produkt")) {
                    z.produktId = attrs.value("produkt").toString().trimmed();
                    if (!knownProducts.isEmpty() && !z.produktId.isEmpty() && !knownProducts.contains(z.produktId)) {
                        QString msg = trBi(
                            QString("Unbekannte Produkt-ID '%1' in Protokoll").arg(z.produktId),
                            QString("Unknown product ID '%1' in log").arg(z.produktId)
                        );
                        proto->warnings.append(msg);
                    }
                }

                if (attrs.hasAttribute("gut")) {
                    bool ok = false;
                    z.gut = attrs.value("gut").toInt(&ok);
                    if (!ok || z.gut < 0) {
                        z.isPlausible = false;
                        QString msg = trBi(
                            QString("Negative/ungültige Gutteile (%1) um %2 – Wert für OEE auf 0 korrigiert")
                                .arg(attrs.value("gut").toString(), z.beginn.toString("HH:mm:ss")),
                            QString("Negative/invalid good parts (%1) at %2 – clamped to 0 for OEE")
                                .arg(attrs.value("gut").toString(), z.beginn.toString("HH:mm:ss"))
                        );
                        if (z.warningMessage.isEmpty()) z.warningMessage = msg;
                        proto->warnings.append(msg);
                        z.gut = 0;
                    }
                }

                if (attrs.hasAttribute("ausschuss")) {
                    bool ok = false;
                    z.ausschuss = attrs.value("ausschuss").toInt(&ok);
                    if (!ok || z.ausschuss < 0) {
                        z.isPlausible = false;
                        QString msg = trBi(
                            QString("Negativer/ungültiger Ausschuss (%1) um %2 – auf 0 korrigiert")
                                .arg(attrs.value("ausschuss").toString(), z.beginn.toString("HH:mm:ss")),
                            QString("Negative/invalid scrap (%1) at %2 – clamped to 0")
                                .arg(attrs.value("ausschuss").toString(), z.beginn.toString("HH:mm:ss"))
                        );
                        if (z.warningMessage.isEmpty()) z.warningMessage = msg;
                        proto->warnings.append(msg);
                        z.ausschuss = 0;
                    }
                }

                if (attrs.hasAttribute("code")) {
                    z.stoerCode = attrs.value("code").toString().trimmed();
                    // z.B. A012_2026-09-22.xml hat code="X-999"
                    if (!knownStoercodes.isEmpty() && !z.stoerCode.isEmpty() && !knownStoercodes.contains(z.stoerCode)) {
                        z.isPlausible = false;
                        QString msg = trBi(
                            QString("Unbekannter Störcode '%1' um %2 (nicht in stoercodes.csv)")
                                .arg(z.stoerCode, z.beginn.toString("HH:mm:ss")),
                            QString("Unknown fault code '%1' at %2 (not in stoercodes.csv)")
                                .arg(z.stoerCode, z.beginn.toString("HH:mm:ss"))
                        );
                        if (z.warningMessage.isEmpty()) z.warningMessage = msg;
                        proto->warnings.append(msg);
                    }
                }

                currentSchicht->zustaende.append(z);
            }
        } else if (token == QXmlStreamReader::EndElement) {
            if (xml.name().toString() == "schicht") {
                currentSchicht = nullptr;
            }
        }
    }

    if (xml.hasError()) {
        // z.B. A045_2026-09-25.xml hat unclosed token in Zeile 33!
        proto->loadSuccess = false;
        proto->errorMessage = trBi(
            QString("XML-Syntaxfehler in Datei '%1' (Zeile %2, Spalte %3): %4")
                .arg(QFileInfo(filePath).fileName())
                .arg(xml.lineNumber())
                .arg(xml.columnNumber())
                .arg(xml.errorString()),
            QString("XML syntax error in file '%1' (Line %2, Column %3): %4")
                .arg(QFileInfo(filePath).fileName())
                .arg(xml.lineNumber())
                .arg(xml.columnNumber())
                .arg(xml.errorString())
        );
        return proto;
    }

    proto->loadSuccess = true;
    return proto;
}

} // namespace RoboCell
