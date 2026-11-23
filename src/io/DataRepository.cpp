#include "io/DataRepository.h"
#include "io/XmlProtokollReader.h"
#include "core/LanguageManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>

namespace RoboCell {

static QString makeIndexKey(const QString& anlageId, const QDate& datum) {
    return anlageId.trimmed() + "|" + datum.toString("yyyy-MM-dd");
}

DataRepository::DataRepository(QObject* parent)
    : QObject(parent), m_protokollCache(128) {}

QString DataRepository::findDefaultDataDirectory() {
    QStringList candidates = {
        "RoboCell_Daten",
        "data",
        "../RoboCell_Daten",
        "../../RoboCell_Daten",
        "../../../RoboCell_Daten"
    };

    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString& rel : candidates) {
        QDir d1(QDir::current().filePath(rel));
        if (d1.exists() && QFile::exists(d1.filePath("anlagen.csv"))) {
            return d1.absolutePath();
        }
        if (!appDir.isEmpty()) {
            QDir d2(QDir(appDir).filePath(rel));
            if (d2.exists() && QFile::exists(d2.filePath("anlagen.csv"))) {
                return d2.absolutePath();
            }
        }
    }
    return QDir("RoboCell_Daten").absolutePath();
}

QStringList DataRepository::splitCsvLine(const QString& line) {
    QStringList fields;
    QString current;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line.at(i + 1) == '"') {
                current.append('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.append(current.trimmed());
            current.clear();
        } else {
            current.append(c);
        }
    }
    fields.append(current.trimmed());
    return fields;
}

void DataRepository::addIssue(ValidationIssue::Severity sev, const QString& file, int line,
                              const QString& id, const QString& de, const QString& en)
{
    ValidationIssue issue;
    issue.severity = sev;
    issue.sourceFile = file;
    issue.lineNumber = line;
    issue.entityId = id;
    issue.messageDe = de;
    issue.messageEn = en;
    m_issues.append(issue);
}

bool DataRepository::loadAllFromDirectory(const QString& dataDir) {
    m_dataDir = dataDir;
    m_linien.clear();
    m_anlagen.clear();
    m_produkte.clear();
    m_stoercodes.clear();
    m_wartungen.clear();
    m_issues.clear();
    m_linienIds.clear();
    m_anlagenIds.clear();
    m_produktIds.clear();
    m_stoercodeIds.clear();
    m_protokollIndex.clear();
    m_datesByAnlage.clear();
    m_protokollCache.clear();

    QDir dir(dataDir);
    if (!dir.exists()) {
        addIssue(ValidationIssue::Severity::Error, dataDir, 0, "-",
                 QString("Datenordner existiert nicht: %1").arg(dataDir),
                 QString("Data directory does not exist: %1").arg(dataDir));
        return false;
    }

    loadLinienCsv(dir.filePath("linien.csv"));
    loadAnlagenCsv(dir.filePath("anlagen.csv"));
    loadProdukteCsv(dir.filePath("produkte.csv"));
    loadStoercodesCsv(dir.filePath("stoercodes.csv"));
    loadWartungenCsv(dir.filePath("wartungen.csv"));
    loadMasterIndexCsv(dir.filePath("master_protokoll_index.csv"));

    emit dataReloaded();
    return !m_anlagen.isEmpty();
}

void DataRepository::loadLinienCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addIssue(ValidationIssue::Severity::Error, "linien.csv", 0, "-",
                 "Datei linien.csv konnte nicht geöffnet werden.",
                 "Could not open file linien.csv.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNo = 0;
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        ++lineNo;
        if (lineNo == 1 || rawLine.isEmpty()) continue;

        const QStringList cols = splitCsvLine(rawLine);
        if (cols.size() < 4) {
            addIssue(ValidationIssue::Severity::Error, "linien.csv", lineNo, "-",
                     QString("Zu wenige Spalten (%1 statt 4)").arg(cols.size()),
                     QString("Too few columns (%1 instead of 4)").arg(cols.size()));
            continue;
        }

        Linie l;
        l.linieId = cols[0];
        l.bezeichnung = cols[1];
        l.halle = cols[2];
        l.rawSchichtmodell = cols[3];
        if (cols[3].toLower() == "3-schicht") {
            l.schichtmodell = Schichtmodell::DreiSchicht;
        } else if (cols[3].toLower() == "2-schicht") {
            l.schichtmodell = Schichtmodell::ZweiSchicht;
        } else {
            l.schichtmodell = Schichtmodell::Unbekannt;
            addIssue(ValidationIssue::Severity::Warning, "linien.csv", lineNo, l.linieId,
                     QString("Unbekanntes Schichtmodell '%1'").arg(cols[3]),
                     QString("Unknown shift model '%1'").arg(cols[3]));
        }

        if (m_linienIds.contains(l.linieId)) {
            addIssue(ValidationIssue::Severity::Warning, "linien.csv", lineNo, l.linieId,
                     QString("Doppelte Linien-ID '%1' ignoriert").arg(l.linieId),
                     QString("Duplicate line ID '%1' ignored").arg(l.linieId));
            continue;
        }

        m_linienIds.insert(l.linieId);
        m_linien.append(l);
    }
}

void DataRepository::loadAnlagenCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addIssue(ValidationIssue::Severity::Error, "anlagen.csv", 0, "-",
                 "Datei anlagen.csv konnte nicht geöffnet werden.",
                 "Could not open file anlagen.csv.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNo = 0;
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        ++lineNo;
        if (lineNo == 1 || rawLine.isEmpty()) continue;

        QStringList cols = splitCsvLine(rawLine);
        const QString candidateId = cols.isEmpty() ? "?" : cols[0];

        // Duplikat-Prüfung (z. B. Zeile 84: A061 doppelt)
        if (m_anlagenIds.contains(candidateId)) {
            addIssue(ValidationIssue::Severity::Warning, "anlagen.csv", lineNo, candidateId,
                     QString("Doppelte anlage_id '%1' ('%2') gefunden – Duplikat wird übersprungen.")
                         .arg(candidateId, cols.value(3)),
                     QString("Duplicate anlage_id '%1' ('%2') found – skipping duplicate.")
                         .arg(candidateId, cols.value(3)));
            continue;
        }

        Anlage a;
        // Spezialbehandlung für Zeile 61 (A060): 11 statt 12 Spalten (traglast_kg fehlt zwischen achsen und baujahr)
        if (cols.size() == 11) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Nur 11 statt 12 Spalten in Zeile %1 (Spalte 'traglast_kg' fehlt) – automatisch mit 0 kg ergänzt.").arg(lineNo);
            QString msgEn = QString("Only 11 instead of 12 columns in line %1 (column 'traglast_kg' missing) – auto-filled with 0 kg.").arg(lineNo);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Warning, "anlagen.csv", lineNo, candidateId, msgDe, msgEn);
            cols.insert(8, "0"); // Füge fehlende traglast_kg an Index 8 ein
        } else if (cols.size() < 12) {
            addIssue(ValidationIssue::Severity::Error, "anlagen.csv", lineNo, candidateId,
                     QString("Ungültige Spaltenanzahl (%1 statt 12) – Zeile übersprungen.").arg(cols.size()),
                     QString("Invalid column count (%1 instead of 12) – line skipped.").arg(cols.size()));
            continue;
        }

        a.anlageId = cols[0];
        a.linieId = cols[1];
        a.position = cols[2].toInt();
        a.bezeichnung = cols[3];
        a.rawTyp = cols[4];
        a.typ = parseAnlagenTyp(cols[4]);
        a.hersteller = cols[5];
        a.modell = cols[6];
        a.achsen = cols[7].toInt();
        a.traglastKg = cols[8].toDouble();
        a.baujahr = cols[9].toInt();

        // Prüfung auf Dezimalkomma statt Punkt (z. B. Zeile 33 A032: "33,2")
        QString taktStr = cols[10];
        if (taktStr.contains(',')) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Dezimalkomma statt Punkt in soll_taktzeit_s ('%1') – automatisch in Punkt konvertiert.").arg(taktStr);
            QString msgEn = QString("Decimal comma instead of dot in soll_taktzeit_s ('%1') – auto-converted to dot.").arg(taktStr);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Warning, "anlagen.csv", lineNo, a.anlageId, msgDe, msgEn);
            taktStr.replace(',', '.');
        }
        bool taktOk = false;
        a.sollTaktzeitS = taktStr.toDouble(&taktOk);

        // Prüfung soll_taktzeit_s > 0 (z. B. Zeile 12 A011 hat soll_taktzeit_s = 0)
        if (!taktOk || a.sollTaktzeitS <= 0.0) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Ungültige Soll-Taktzeit (%1 s <= 0) – bitte in den Stammdaten korrigieren!").arg(cols[10]);
            QString msgEn = QString("Invalid target cycle time (%1 s <= 0) – please correct in master data!").arg(cols[10]);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Error, "anlagen.csv", lineNo, a.anlageId, msgDe, msgEn);
        }

        // Prüfung ob linie_id in linien.csv existiert (z. B. Zeile 41 A040 hat L9)
        if (!m_linienIds.contains(a.linieId)) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Fremdschlüsselfehler: linie_id '%1' existiert nicht in linien.csv (Bezeichnung: '%2').")
                                .arg(a.linieId, a.bezeichnung);
            QString msgEn = QString("Foreign key error: linie_id '%1' does not exist in linien.csv (Label: '%2').")
                                .arg(a.linieId, a.bezeichnung);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Error, "anlagen.csv", lineNo, a.anlageId, msgDe, msgEn);
        }

        // Prüfung AnlagenTyp (z. B. Zeile 64 A063 hat Tippfehler 'schweisroboter')
        if (a.typ == AnlagenTyp::Unbekannt) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Unbekannter/fehlerhafter Anlagentyp '%1' (Bezeichnung: '%2').")
                                .arg(a.rawTyp, a.bezeichnung);
            QString msgEn = QString("Unknown/misspelled machine type '%1' (Label: '%2').")
                                .arg(a.rawTyp, a.bezeichnung);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Warning, "anlagen.csv", lineNo, a.anlageId, msgDe, msgEn);
        }

        // Prüfung Baujahr plausibel (z. B. Zeile 65 A064 hat Baujahr 2031)
        if (a.baujahr < 1980 || a.baujahr > 2026) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Unplausibles Baujahr %1 (liegt außerhalb 1980–2026).").arg(a.baujahr);
            QString msgEn = QString("Implausible build year %1 (outside 1980–2026).").arg(a.baujahr);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Warning, "anlagen.csv", lineNo, a.anlageId, msgDe, msgEn);
        }

        a.rawStatus = cols[11];
        a.status = parseAnlagenStatus(cols[11]);
        if (a.status == AnlagenStatus::Unbekannt) {
            a.hasValidationWarning = true;
            QString msgDe = QString("Unbekannter Anlagenstatus '%1'.").arg(a.rawStatus);
            QString msgEn = QString("Unknown machine status '%1'.").arg(a.rawStatus);
            a.validationMessages.append(msgDe);
            addIssue(ValidationIssue::Severity::Warning, "anlagen.csv", lineNo, a.anlageId, msgDe, msgEn);
        }

        m_anlagenIds.insert(a.anlageId);
        m_anlagen.append(a);
    }
}

void DataRepository::loadProdukteCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addIssue(ValidationIssue::Severity::Warning, "produkte.csv", 0, "-",
                 "Datei produkte.csv konnte nicht geöffnet werden.",
                 "Could not open file produkte.csv.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNo = 0;
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        ++lineNo;
        if (lineNo == 1 || rawLine.isEmpty()) continue;

        const QStringList cols = splitCsvLine(rawLine);
        if (cols.size() < 3) continue;

        Produkt p;
        p.produktId = cols[0];
        p.linieId = cols[1];
        p.bezeichnung = cols[2];

        // Prüfung ob linie_id existiert (z. B. Zeile 22 P-7111 hat L7)
        if (!m_linienIds.contains(p.linieId)) {
            p.hasValidationWarning = true;
            p.validationMessage = QString("linie_id '%1' existiert nicht in linien.csv").arg(p.linieId);
            addIssue(ValidationIssue::Severity::Warning, "produkte.csv", lineNo, p.produktId,
                     QString("Produkt '%1' verweist auf nicht existierende Linie '%2'.").arg(p.produktId, p.linieId),
                     QString("Product '%1' references non-existent line '%2'.").arg(p.produktId, p.linieId));
        }

        m_produktIds.insert(p.produktId);
        m_produkte.append(p);
    }
}

void DataRepository::loadStoercodesCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addIssue(ValidationIssue::Severity::Error, "stoercodes.csv", 0, "-",
                 "Datei stoercodes.csv konnte nicht geöffnet werden.",
                 "Could not open file stoercodes.csv.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNo = 0;
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        ++lineNo;
        if (lineNo == 1 || rawLine.isEmpty()) continue;

        const QStringList cols = splitCsvLine(rawLine);
        if (cols.size() < 3) continue;

        Stoercode sc;
        sc.code = cols[0];
        sc.kategorie = cols[1];
        sc.beschreibung = cols[2];

        // Duplikat-Prüfung (z. B. Zeile 26: M-102 doppelt)
        if (m_stoercodeIds.contains(sc.code)) {
            addIssue(ValidationIssue::Severity::Warning, "stoercodes.csv", lineNo, sc.code,
                     QString("Doppelter Störcode '%1' ('%2') – erster Eintrag wird beibehalten.")
                         .arg(sc.code, sc.beschreibung),
                     QString("Duplicate fault code '%1' ('%2') – keeping first entry.")
                         .arg(sc.code, sc.beschreibung));
            continue;
        }

        m_stoercodeIds.insert(sc.code);
        m_stoercodes.append(sc);
    }
}

void DataRepository::loadWartungenCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addIssue(ValidationIssue::Severity::Warning, "wartungen.csv", 0, "-",
                 "Datei wartungen.csv konnte nicht geöffnet werden.",
                 "Could not open file wartungen.csv.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNo = 0;
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        ++lineNo;
        if (lineNo == 1 || rawLine.isEmpty()) continue;

        const QStringList cols = splitCsvLine(rawLine);
        if (cols.size() < 7) continue;

        Wartung w;
        w.wartungId = cols[0];
        w.anlageId = cols[1];
        w.datum = QDate::fromString(cols[2], "yyyy-MM-dd");
        w.art = cols[3];
        w.dauerMin = cols[4].toInt();
        w.geplant = (cols[5].toLower() == "ja" || cols[5].toLower() == "true");
        w.bemerkung = cols[6];

        // Fremdschlüssel-Prüfung (z. B. Zeile 20 W0019 verweist auf A199)
        if (!m_anlagenIds.contains(w.anlageId)) {
            w.hasValidationWarning = true;
            w.validationMessage = QString("anlage_id '%1' existiert nicht in anlagen.csv").arg(w.anlageId);
            addIssue(ValidationIssue::Severity::Warning, "wartungen.csv", lineNo, w.wartungId,
                     QString("Wartung '%1' verweist auf unbekannte Anlage '%2'.").arg(w.wartungId, w.anlageId),
                     QString("Maintenance '%1' references unknown machine '%2'.").arg(w.wartungId, w.anlageId));
        }

        m_wartungen.append(w);
    }
}

void DataRepository::loadMasterIndexCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addIssue(ValidationIssue::Severity::Error, "master_protokoll_index.csv", 0, "-",
                 "Datei master_protokoll_index.csv konnte nicht geöffnet werden.",
                 "Could not open file master_protokoll_index.csv.");
        return;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    int lineNo = 0;
    while (!in.atEnd()) {
        const QString rawLine = in.readLine().trimmed();
        ++lineNo;
        if (lineNo == 1 || rawLine.isEmpty()) continue;

        const QStringList cols = splitCsvLine(rawLine);
        if (cols.size() < 4) continue;

        ProtokollIndexEntry entry;
        entry.anlageId = cols[0];
        entry.datum = QDate::fromString(cols[1], "yyyy-MM-dd");
        entry.relativerPfad = cols[2];
        entry.anzahlZustaende = cols[3].toInt();

        if (!entry.datum.isValid()) continue;

        m_protokollIndex.insert(makeIndexKey(entry.anlageId, entry.datum), entry);
        m_datesByAnlage[entry.anlageId].append(entry.datum);
    }

    for (auto it = m_datesByAnlage.begin(); it != m_datesByAnlage.end(); ++it) {
        std::sort(it.value().begin(), it.value().end());
    }
}

const Linie* DataRepository::findLinie(const QString& linieId) const {
    for (const auto& l : m_linien) {
        if (l.linieId == linieId) return &l;
    }
    return nullptr;
}

const Anlage* DataRepository::findAnlage(const QString& anlageId) const {
    for (const auto& a : m_anlagen) {
        if (a.anlageId == anlageId) return &a;
    }
    return nullptr;
}

Anlage* DataRepository::findAnlageMutable(const QString& anlageId) {
    for (auto& a : m_anlagen) {
        if (a.anlageId == anlageId) return &a;
    }
    return nullptr;
}

const Produkt* DataRepository::findProdukt(const QString& produktId) const {
    for (const auto& p : m_produkte) {
        if (p.produktId == produktId) return &p;
    }
    return nullptr;
}

const Stoercode* DataRepository::findStoercode(const QString& code) const {
    for (const auto& sc : m_stoercodes) {
        if (sc.code == code) return &sc;
    }
    return nullptr;
}

QList<Wartung> DataRepository::wartungenForAnlage(const QString& anlageId) const {
    QList<Wartung> result;
    for (const auto& w : m_wartungen) {
        if (w.anlageId == anlageId) result.append(w);
    }
    return result;
}

bool DataRepository::updateAnlage(const Anlage& updated) {
    for (auto& a : m_anlagen) {
        if (a.anlageId == updated.anlageId) {
            a = updated;
            a.isModified = true;
            // Re-validate after user edit
            a.validationMessages.clear();
            if (a.sollTaktzeitS <= 0.0) {
                a.validationMessages.append("Ungültige Soll-Taktzeit (<= 0)");
            }
            if (!m_linienIds.contains(a.linieId)) {
                a.validationMessages.append(QString("Unbekannte Linie '%1'").arg(a.linieId));
            }
            if (a.baujahr < 1980 || a.baujahr > 2026) {
                a.validationMessages.append(QString("Unplausibles Baujahr %1").arg(a.baujahr));
            }
            if (a.typ == AnlagenTyp::Unbekannt) {
                a.validationMessages.append("Unbekannter Anlagentyp");
            }
            a.hasValidationWarning = !a.validationMessages.isEmpty();
            emit anlageUpdated(a.anlageId);
            return true;
        }
    }
    return false;
}

void DataRepository::addWartung(const Wartung& newWartung) {
    m_wartungen.append(newWartung);
    emit wartungAdded(newWartung.anlageId);
}

QList<QDate> DataRepository::availableDatesForAnlage(const QString& anlageId) const {
    return m_datesByAnlage.value(anlageId);
}

QList<QDate> DataRepository::allAvailableDates() const {
    QSet<QDate> dateSet;
    for (auto it = m_datesByAnlage.cbegin(); it != m_datesByAnlage.cend(); ++it) {
        for (const QDate& d : it.value()) {
            dateSet.insert(d);
        }
    }
    QList<QDate> dates = dateSet.values();
    std::sort(dates.begin(), dates.end());
    return dates;
}

QString DataRepository::relativeXmlPath(const QString& anlageId, const QDate& datum) const {
    const QString key = makeIndexKey(anlageId, datum);
    if (m_protokollIndex.contains(key)) {
        return m_protokollIndex.value(key).relativerPfad;
    }
    return QString();
}

std::shared_ptr<Schichtprotokoll> DataRepository::getProtokollLazy(const QString& anlageId, const QDate& datum) {
    const QString key = makeIndexKey(anlageId, datum);
    if (auto cached = m_protokollCache.get(key)) {
        return *cached;
    }

    if (!m_protokollIndex.contains(key)) {
        auto missing = std::make_shared<Schichtprotokoll>();
        missing->anlageId = anlageId;
        missing->datum = datum;
        missing->loadSuccess = false;
        missing->errorMessage = trBi(
            QString("Kein Index-Eintrag in master_protokoll_index.csv für Anlage %1 am %2 gefunden.")
                .arg(anlageId, datum.toString("yyyy-MM-dd")),
            QString("No index entry in master_protokoll_index.csv found for machine %1 on %2.")
                .arg(anlageId, datum.toString("yyyy-MM-dd"))
        );
        return missing;
    }

    const QString relPath = m_protokollIndex.value(key).relativerPfad;
    const QString fullPath = QDir(m_dataDir).filePath(relPath);
    auto proto = XmlProtokollReader::readFromFile(fullPath, m_produktIds, m_stoercodeIds);
    if (proto->anlageId.isEmpty()) proto->anlageId = anlageId;
    if (!proto->datum.isValid()) proto->datum = datum;

    m_protokollCache.put(key, proto);
    return proto;
}

QList<std::shared_ptr<Schichtprotokoll>> DataRepository::loadAllProtokolleForAnlage(const QString& anlageId) {
    QList<std::shared_ptr<Schichtprotokoll>> list;
    const QList<QDate> dates = availableDatesForAnlage(anlageId);
    for (const QDate& d : dates) {
        list.append(getProtokollLazy(anlageId, d));
    }
    return list;
}

bool DataRepository::exportModifiedCsv(const QString& targetDir, QString* errorOut) const {
    QDir dir(targetDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        if (errorOut) *errorOut = trBi("Zielordner konnte nicht erstellt werden.", "Could not create target directory.");
        return false;
    }

    // 1. Export anlagen_out.csv
    {
        QFile f(dir.filePath("anlagen_out.csv"));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (errorOut) *errorOut = f.errorString();
            return false;
        }
        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);
        out << "anlage_id,linie_id,position,bezeichnung,typ,hersteller,modell,achsen,traglast_kg,baujahr,soll_taktzeit_s,status\n";
        for (const auto& a : m_anlagen) {
            out << a.anlageId << ","
                << a.linieId << ","
                << a.position << ","
                << a.bezeichnung << ","
                << anlagenTypToCsv(a.typ, a.rawTyp) << ","
                << a.hersteller << ","
                << a.modell << ","
                << a.achsen << ","
                << a.traglastKg << ","
                << a.baujahr << ","
                << QString::number(a.sollTaktzeitS, 'f', 1) << ","
                << anlagenStatusToCsv(a.status, a.rawStatus) << "\n";
        }
    }

    // 2. Export wartungen_out.csv
    {
        QFile f(dir.filePath("wartungen_out.csv"));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (errorOut) *errorOut = f.errorString();
            return false;
        }
        QTextStream out(&f);
        out.setEncoding(QStringConverter::Utf8);
        out << "wartung_id,anlage_id,datum,art,dauer_min,geplant,bemerkung\n";
        for (const auto& w : m_wartungen) {
            out << w.wartungId << ","
                << w.anlageId << ","
                << w.datum.toString("yyyy-MM-dd") << ","
                << w.art << ","
                << w.dauerMin << ","
                << (w.geplant ? "ja" : "nein") << ","
                << w.bemerkung << "\n";
        }
    }

    return true;
}

bool DataRepository::loadFromSqlite(const QString& sqliteFilePath) {
    if (!QFile::exists(sqliteFilePath)) return false;

    const QString connName = "RoboCellSqliteReadConn";
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(sqliteFilePath);
        if (db.open()) {
            m_anlagen.clear();
            m_anlagenIds.clear();
            QSqlQuery q(db);
            if (q.exec("SELECT anlage_id, linie_id, position, bezeichnung, typ, hersteller, modell, achsen, traglast_kg, baujahr, soll_taktzeit_s, status FROM anlagen")) {
                while (q.next()) {
                    Anlage a;
                    a.anlageId = q.value(0).toString();
                    if (m_anlagenIds.contains(a.anlageId)) continue;
                    a.linieId = q.value(1).toString();
                    a.position = q.value(2).toInt();
                    a.bezeichnung = q.value(3).toString();
                    a.rawTyp = q.value(4).toString();
                    a.typ = parseAnlagenTyp(a.rawTyp);
                    a.hersteller = q.value(5).toString();
                    a.modell = q.value(6).toString();
                    a.achsen = q.value(7).toInt();
                    a.traglastKg = q.value(8).toDouble();
                    a.baujahr = q.value(9).toInt();
                    QString taktStr = q.value(10).toString().replace(',', '.');
                    a.sollTaktzeitS = taktStr.toDouble();
                    a.rawStatus = q.value(11).toString();
                    a.status = parseAnlagenStatus(a.rawStatus);

                    if (a.sollTaktzeitS <= 0.0 || !m_linienIds.contains(a.linieId) ||
                        a.typ == AnlagenTyp::Unbekannt || a.baujahr > 2026) {
                        a.hasValidationWarning = true;
                        a.validationMessages.append("Aus SQLite geladen: Plausibilitätswarnung vorhanden.");
                    }
                    m_anlagenIds.insert(a.anlageId);
                    m_anlagen.append(a);
                }
                ok = !m_anlagen.isEmpty();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    if (ok) emit dataReloaded();
    return ok;
}

bool DataRepository::saveToSqlite(const QString& sqliteFilePath, QString* errorOut) const {
    const QString connName = "RoboCellSqliteWriteConn";
    bool success = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
        db.setDatabaseName(sqliteFilePath);
        if (!db.open()) {
            if (errorOut) *errorOut = db.lastError().text();
        } else {
            QSqlQuery q(db);
            db.transaction();
            q.exec("CREATE TABLE IF NOT EXISTS anlagen ("
                   "anlage_id TEXT PRIMARY KEY, linie_id TEXT, position INTEGER, bezeichnung TEXT, "
                   "typ TEXT, hersteller TEXT, modell TEXT, achsen INTEGER, traglast_kg REAL, "
                   "baujahr INTEGER, soll_taktzeit_s REAL, status TEXT)");
            q.exec("DELETE FROM anlagen");

            q.prepare("INSERT INTO anlagen VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            for (const auto& a : m_anlagen) {
                q.addBindValue(a.anlageId);
                q.addBindValue(a.linieId);
                q.addBindValue(a.position);
                q.addBindValue(a.bezeichnung);
                q.addBindValue(anlagenTypToCsv(a.typ, a.rawTyp));
                q.addBindValue(a.hersteller);
                q.addBindValue(a.modell);
                q.addBindValue(a.achsen);
                q.addBindValue(a.traglastKg);
                q.addBindValue(a.baujahr);
                q.addBindValue(a.sollTaktzeitS);
                q.addBindValue(anlagenStatusToCsv(a.status, a.rawStatus));
                q.exec();
            }

            q.exec("CREATE TABLE IF NOT EXISTS wartungen ("
                   "wartung_id TEXT PRIMARY KEY, anlage_id TEXT, datum TEXT, art TEXT, "
                   "dauer_min INTEGER, geplant TEXT, bemerkung TEXT)");
            q.exec("DELETE FROM wartungen");
            q.prepare("INSERT INTO wartungen VALUES (?, ?, ?, ?, ?, ?, ?)");
            for (const auto& w : m_wartungen) {
                q.addBindValue(w.wartungId);
                q.addBindValue(w.anlageId);
                q.addBindValue(w.datum.toString("yyyy-MM-dd"));
                q.addBindValue(w.art);
                q.addBindValue(w.dauerMin);
                q.addBindValue(w.geplant ? "ja" : "nein");
                q.addBindValue(w.bemerkung);
                q.exec();
            }

            success = db.commit();
            if (!success && errorOut) *errorOut = db.lastError().text();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return success;
}

} // namespace RoboCell
