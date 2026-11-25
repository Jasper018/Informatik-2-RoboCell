#ifndef OEEBERECHNUNG_H
#define OEEBERECHNUNG_H

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include "model/datentypen.h"

// Ein Punkt der Kurve "kumulierte Stueckzahlen"
struct Verlaufspunkt {
    QDateTime zeit;
    int gut = 0;
    int ausschuss = 0;
};

// Alle Kennzahlen fuer einen Tag oder eine einzelne Schicht
struct OeeKennzahlen {
    QString anlageId;
    QDate datum;
    QString bezeichnung;              // z.B. Datum oder Schichtname
    bool gueltig = false;             // false: keine Auswertung moeglich
    bool hatGeplanteSchicht = false;

    // Zeiten in Sekunden
    qint64 planzeitSek = 0;           // T_plan (ohne pause und wartung)
    qint64 laufzeitSek = 0;           // T_lauf (nur produktion)
    qint64 schichtzeitSek = 0;        // alle Zustaende in geplanten Schichten
    QMap<ZustandTyp, qint64> zustandSek;

    // Stueckzahlen
    int gut = 0;
    int ausschuss = 0;
    int gesamt() const { return gut + ausschuss; }

    // Faktoren (0.0 bis 1.0)
    double verfuegbarkeit = 0.0;      // V
    double leistungsgrad = 0.0;       // L
    double qualitaet = 0.0;           // Q
    double oee = 0.0;                 // V * L * Q

    // Zuverlaessigkeit
    int anzahlStoerungen = 0;
    qint64 stoerzeitSek = 0;
    double mtbfMin = 0.0;
    double mttrMin = 0.0;
    QMap<QString, int> codeAnzahl;
    QMap<QString, qint64> codeSek;

    QList<Verlaufspunkt> verlauf;     // nur fuer die Tagesansicht
};

// Ergebnis der 14-Tage-Analyse einer Anlage
struct Trendbericht {
    Q_DECLARE_TR_FUNCTIONS(Trendbericht)

public:
    QString anlageId;
    bool auffaellig = false;
    bool sinkendeLeistung = false;
    bool steigenderAusschuss = false;
    bool niedrigeOee = false;
    bool haeufigerCode = false;
    QString haeufigsterCode;
    int haeufigsterCodeAnzahl = 0;
    double leistungWoche1 = 0.0;
    double leistungWoche2 = 0.0;
    double ausschussWoche1 = 0.0;
    double ausschussWoche2 = 0.0;
    double mittelOee = 0.0;
    double mittelVerfuegbarkeit = 0.0;
    double mittelLeistung = 0.0;
    double mittelQualitaet = 0.0;
    double mtbfMin = 0.0;
    double mttrMin = 0.0;
    qint64 stoerzeitSek = 0;
    int gueltigeTage = 0;

    // Lesbare Hinweise in der aktuellen Sprache
    QStringList hinweise() const;
};

// Engpass-Eintrag einer Station innerhalb einer Linie
struct EngpassEintrag {
    QString anlageId;
    QString bezeichnung;
    int position = 0;
    double sollTaktzeitS = 0.0;
    double stoerzeitMin = 0.0;
    double verursachterMangelMin = 0.0;    // Materialmangel der nachfolgenden Anlage
    double verursachterRueckstauMin = 0.0; // Rueckstau der vorgelagerten Anlage
    double score = 0.0;
    bool hauptEngpass = false;
};

// Eintrag der Pareto-Auswertung der Stoercodes
struct ParetoEintrag {
    QString code;
    int anzahl = 0;
    double minuten = 0.0;
    double kumuliertProzent = 0.0;
};

class OeeBerechnung {
public:
    // Kennzahlen eines kompletten Tages
    static OeeKennzahlen tag(const Schichtprotokoll& protokoll, double sollTaktzeitS);

    // Kennzahlen je Schicht (Schichtvergleich)
    static QList<OeeKennzahlen> schichten(const Schichtprotokoll& protokoll, double sollTaktzeitS);

    // Trend ueber alle vorhandenen Tage einer Anlage
    static Trendbericht trend(const Anlage& anlage, const QList<OeeKennzahlen>& tage);

    // Engpass: stationen und kennzahlen haben dieselbe Reihenfolge (nach Position)
    static QList<EngpassEintrag> engpass(const QList<Anlage>& stationen,
                                         const QList<OeeKennzahlen>& kennzahlen);

    // Pareto ueber beliebig viele Tageskennzahlen
    static QList<ParetoEintrag> pareto(const QList<OeeKennzahlen>& tage);

private:
    static void schichtAddieren(const Schicht& schicht, OeeKennzahlen* k,
                                int* summeGut, int* summeAusschuss);
    static void faktorenBerechnen(OeeKennzahlen* k, double sollTaktzeitS);
};

#endif // OEEBERECHNUNG_H
