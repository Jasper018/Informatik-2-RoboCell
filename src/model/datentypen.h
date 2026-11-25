#ifndef DATENTYPEN_H
#define DATENTYPEN_H

#include <QString>
#include <QStringList>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <memory>
#include "problem.h"

// ---------------------------------------------------------------------------
// Aufzaehlungen
// ---------------------------------------------------------------------------
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

enum class AnlagenStatus {
    InBetrieb,
    Stoerung,
    Wartung,
    Umbau,
    Unbekannt
};

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

// Umwandlung zwischen Text (CSV/XML) und Aufzaehlung
AnlagenTyp anlagenTypVonText(const QString& text);
QString anlagenTypText(AnlagenTyp typ);
QString anlagenTypName(AnlagenTyp typ, const QString& roh = QString());
AnlagenStatus anlagenStatusVonText(const QString& text);
QString anlagenStatusText(AnlagenStatus status);
QString anlagenStatusName(AnlagenStatus status, const QString& roh = QString());
ZustandTyp zustandTypVonText(const QString& text);
QString zustandName(ZustandTyp typ);   // Anzeigename in der aktuellen Sprache

// ---------------------------------------------------------------------------
// Stammdaten (CSV)
// ---------------------------------------------------------------------------
struct Linie {
    QString linieId;
    QString bezeichnung;
    QString halle;
    QString schichtmodell;   // "3-schicht" oder "2-schicht"
};

struct Anlage {
    QString anlageId;
    QString linieId;
    int position = 0;
    QString bezeichnung;
    AnlagenTyp typ = AnlagenTyp::Unbekannt;
    QString typRoh;          // Originaltext aus der CSV
    QString hersteller;
    QString modell;
    int achsen = 0;
    double traglastKg = 0.0;
    int baujahr = 0;
    double sollTaktzeitS = 0.0;
    AnlagenStatus status = AnlagenStatus::Unbekannt;
    QString statusRoh;       // Originaltext aus der CSV
    bool bearbeitet = false;
    QList<Problem> probleme; // Flags aus der Pruefung beim Laden

    int anzahlProbleme(Schweregrad grad) const;
    bool hatProbleme() const { return !probleme.isEmpty(); }
};

struct Produkt {
    QString produktId;
    QString linieId;
    QString bezeichnung;
    QList<Problem> probleme;
};

struct Stoercode {
    QString code;
    QString kategorie;
    QString beschreibung;
};

struct Wartung {
    QString wartungId;
    QString anlageId;
    QDate datum;
    QString art;
    int dauerMin = 0;
    bool geplant = true;
    QString bemerkung;
    QList<Problem> probleme;
};

// ---------------------------------------------------------------------------
// Protokolle (XML)
// ---------------------------------------------------------------------------
struct ProtokollIndexEintrag {
    QString anlageId;
    QDate datum;
    QString pfad;            // relativ zum Datenordner
    int anzahlZustaende = 0;
    bool geprueft = false;
    QList<Problem> probleme; // Flags aus der Pruefung beim Laden
};

struct ZustandEintrag {
    ZustandTyp typ = ZustandTyp::Unbekannt;
    QString typRoh;
    QDateTime beginn;
    QDateTime ende;
    QString produktId;
    int gut = 0;
    int ausschuss = 0;
    QString stoerCode;
    bool plausibel = true;

    // Dauer in Sekunden (0 bei ungueltigem Intervall)
    qint64 dauerSekunden() const;
};

struct Schicht {
    QString name;            // frueh, spaet, nacht
    QDateTime beginn;
    QDateTime ende;
    bool geplant = false;
    QList<ZustandEintrag> zustaende;
};

struct Schichtprotokoll {
    QString anlageId;
    QDate datum;
    QString quellPfad;
    QList<Schicht> schichten;
    bool geladen = false;         // false bei fehlender oder defekter Datei
    QList<Problem> probleme;      // Flags dieses Protokolls
    int anzahlZustaende() const;
};

#endif // DATENTYPEN_H
