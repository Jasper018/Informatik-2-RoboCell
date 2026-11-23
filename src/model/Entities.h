#pragma once

#include <QString>
#include <QStringList>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <memory>

namespace RoboCell {

/**
 * @brief Schichtmodell einer Fertigungslinie
 */
enum class Schichtmodell {
    ZweiSchicht,
    DreiSchicht,
    Unbekannt
};

/**
 * @brief Typ einer automatisierten Anlage / Station
 */
enum class AnlagenTyp {
    CncBeladung,
    Schweissroboter,
    Montageroboter,
    Schraubstation,
    Klebestation,
    Pruefstation,
    Laserbeschriftung,
    Palettierer,
    Unbekannt
};

/**
 * @brief Aktueller Betriebszustand (Stammdaten) einer Anlage
 */
enum class AnlagenStatus {
    InBetrieb,
    Stoerung,
    Wartung,
    Umbau,
    Unbekannt
};

/**
 * @brief Zustandstyp innerhalb eines Schichtprotokolls (XML)
 */
enum class ZustandTyp {
    Produktion,
    Ruesten,
    Stoerung,
    Materialmangel,
    Rueckstau,
    Wartung,
    Pause,
    Unbekannt
};

/**
 * @brief Repräsentiert eine Fertigungslinie (linien.csv)
 */
struct Linie {
    QString linieId;
    QString bezeichnung;
    QString halle;
    Schichtmodell schichtmodell = Schichtmodell::DreiSchicht;
    QString rawSchichtmodell;
};

/**
 * @brief Repräsentiert eine Roboterzelle oder Station (anlagen.csv)
 */
struct Anlage {
    QString anlageId;
    QString linieId;
    int position = 0;
    QString bezeichnung;
    AnlagenTyp typ = AnlagenTyp::Unbekannt;
    QString rawTyp;
    QString hersteller;
    QString modell;
    int achsen = 0;
    double traglastKg = 0.0;
    int baujahr = 2020;
    double sollTaktzeitS = 0.0;
    AnlagenStatus status = AnlagenStatus::InBetrieb;
    QString rawStatus;

    // Datenqualitäts- und Plausibilitäts-Informationen
    bool hasValidationWarning = false;
    QStringList validationMessages;
    bool isModified = false;
};

/**
 * @brief Repräsentiert ein Produkt (produkte.csv)
 */
struct Produkt {
    QString produktId;
    QString linieId;
    QString bezeichnung;
    bool hasValidationWarning = false;
    QString validationMessage;
};

/**
 * @brief Repräsentiert einen Störcode (stoercodes.csv)
 */
struct Stoercode {
    QString code;
    QString kategorie;
    QString beschreibung;
};

/**
 * @brief Repräsentiert einen Wartungseintrag (wartungen.csv)
 */
struct Wartung {
    QString wartungId;
    QString anlageId;
    QDate datum;
    QString art;
    int dauerMin = 0;
    bool geplant = true;
    QString bemerkung;
    bool hasValidationWarning = false;
    QString validationMessage;
};

/**
 * @brief Index-Eintrag aus master_protokoll_index.csv bzw. protokoll_index.csv
 */
struct ProtokollIndexEntry {
    QString anlageId;
    QDate datum;
    QString relativerPfad;
    int anzahlZustaende = 0;
};

/**
 * @brief Einzelner Zustandseintrag in einer Schicht (XML <zustand .../>)
 */
struct ZustandEintrag {
    ZustandTyp typ = ZustandTyp::Unbekannt;
    QString rawTyp;
    QDateTime beginn;
    QDateTime ende;
    QString produktId;
    int gut = 0;
    int ausschuss = 0;
    QString stoerCode;
    bool isPlausible = true;
    QString warningMessage;

    qint64 dauerSekunden() const {
        if (!beginn.isValid() || !ende.isValid()) return 0;
        qint64 secs = beginn.secsTo(ende);
        return secs > 0 ? secs : 0;
    }
};

/**
 * @brief Eine Schicht eines Tages (frueh, spaet, nacht)
 */
struct Schicht {
    QString name; // "frueh", "spaet", "nacht"
    QDateTime beginn;
    QDateTime ende;
    bool geplant = false;
    QList<ZustandEintrag> zustaende;
};

/**
 * @brief Vollständiges Tages-Schichtprotokoll einer Anlage
 */
struct Schichtprotokoll {
    QString anlageId;
    QDate datum;
    QString quellPfad;
    QList<Schicht> schichten;
    bool loadSuccess = false;
    QString errorMessage;
    QStringList warnings;
};

/**
 * @brief Eintrag im Lade-/Plausibilitäts-Bericht
 */
struct ValidationIssue {
    enum class Severity { Info, Warning, Error };
    Severity severity = Severity::Warning;
    QString sourceFile;
    int lineNumber = 0;
    QString entityId;
    QString messageDe;
    QString messageEn;
};

// Hilfsfunktionen für Konvertierung & Anzeige
inline AnlagenTyp parseAnlagenTyp(const QString& s) {
    const QString t = s.trimmed().toLower();
    if (t == "cnc_beladung") return AnlagenTyp::CncBeladung;
    if (t == "schweissroboter") return AnlagenTyp::Schweissroboter;
    if (t == "montageroboter") return AnlagenTyp::Montageroboter;
    if (t == "schraubstation") return AnlagenTyp::Schraubstation;
    if (t == "klebestation") return AnlagenTyp::Klebestation;
    if (t == "pruefstation") return AnlagenTyp::Pruefstation;
    if (t == "laserbeschriftung") return AnlagenTyp::Laserbeschriftung;
    if (t == "palettierer") return AnlagenTyp::Palettierer;
    return AnlagenTyp::Unbekannt;
}

inline QString anlagenTypToCsv(AnlagenTyp t, const QString& fallback = QString()) {
    switch (t) {
        case AnlagenTyp::CncBeladung: return "cnc_beladung";
        case AnlagenTyp::Schweissroboter: return "schweissroboter";
        case AnlagenTyp::Montageroboter: return "montageroboter";
        case AnlagenTyp::Schraubstation: return "schraubstation";
        case AnlagenTyp::Klebestation: return "klebestation";
        case AnlagenTyp::Pruefstation: return "pruefstation";
        case AnlagenTyp::Laserbeschriftung: return "laserbeschriftung";
        case AnlagenTyp::Palettierer: return "palettierer";
        case AnlagenTyp::Unbekannt: return fallback.isEmpty() ? "unbekannt" : fallback;
    }
    return "unbekannt";
}

inline AnlagenStatus parseAnlagenStatus(const QString& s) {
    const QString st = s.trimmed().toLower();
    if (st == "in_betrieb") return AnlagenStatus::InBetrieb;
    if (st == "stoerung") return AnlagenStatus::Stoerung;
    if (st == "wartung") return AnlagenStatus::Wartung;
    if (st == "umbau") return AnlagenStatus::Umbau;
    return AnlagenStatus::Unbekannt;
}

inline QString anlagenStatusToCsv(AnlagenStatus st, const QString& fallback = QString()) {
    switch (st) {
        case AnlagenStatus::InBetrieb: return "in_betrieb";
        case AnlagenStatus::Stoerung: return "stoerung";
        case AnlagenStatus::Wartung: return "wartung";
        case AnlagenStatus::Umbau: return "umbau";
        case AnlagenStatus::Unbekannt: return fallback.isEmpty() ? "in_betrieb" : fallback;
    }
    return "in_betrieb";
}

inline ZustandTyp parseZustandTyp(const QString& s) {
    const QString z = s.trimmed().toLower();
    if (z == "produktion") return ZustandTyp::Produktion;
    if (z == "ruesten") return ZustandTyp::Ruesten;
    if (z == "stoerung") return ZustandTyp::Stoerung;
    if (z == "materialmangel") return ZustandTyp::Materialmangel;
    if (z == "rueckstau") return ZustandTyp::Rueckstau;
    if (z == "wartung") return ZustandTyp::Wartung;
    if (z == "pause") return ZustandTyp::Pause;
    return ZustandTyp::Unbekannt;
}

} // namespace RoboCell
