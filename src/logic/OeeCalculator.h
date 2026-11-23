#pragma once

#include "model/Entities.h"
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QPair>

namespace RoboCell {

class DataRepository;

/**
 * @brief Punkt für die Zeitreihen-Visualisierung kumulierter Stückzahlen
 */
struct CumulativePartPoint {
    QDateTime timestamp;
    int cumulativeGood = 0;
    int cumulativeScrap = 0;
};

/**
 * @brief Vollständige OEE- und Zuverlässigkeitskennzahlen für einen Tag, eine Schicht oder einen Zeitraum
 */
struct OeeMetrics {
    QDate datum;
    QString label; // z.B. "Gesamttag", "Frühschicht" oder Datum
    bool isValid = false;
    bool hasPlannedShifts = false;

    // Zeiten in Sekunden
    qint64 planbelegungszeitSek = 0; // T_plan (ohne pause & wartung)
    qint64 laufzeitSek = 0;          // T_lauf (nur produktion)
    qint64 gesamtSchichtzeitSek = 0;

    // Zeitanteile je Zustand in Sekunden
    QMap<ZustandTyp, qint64> sekundenJeZustand;

    // Stückzahlen
    int gutteile = 0;
    int ausschuss = 0;
    int gesamtteile() const { return gutteile + ausschuss; }

    // OEE-Faktoren (0.0 .. 1.0, bzw. Prozent = * 100.0)
    double verfuegbarkeit = 0.0; // V
    double leistungsgrad = 0.0;  // L
    double qualitaetsrate = 0.0; // Q
    double oee = 0.0;            // V * L * Q

    // Eigene Erweiterung: Zuverlässigkeitskennzahlen MTBF & MTTR
    int anzahlStoerungen = 0;
    qint64 stoerzeitSek = 0;
    double mtbfMinuten = 0.0; // Mittlere Laufzeit zwischen zwei Störungen in Minuten
    double mttrMinuten = 0.0; // Mittlere Stördauer (Reparaturzeit) in Minuten

    // Störcode-Häufigkeiten
    QMap<QString, int> stoercodeCount;
    QMap<QString, qint64> stoercodeSeconds;

    // Verlauf der kumulierten Stückzahlen
    QList<CumulativePartPoint> cumulativePoints;
};

/**
 * @brief Ergebnis der 14-Tage-Trend- und Anomalieanalyse (Erweiterung 6)
 */
struct AnomalyReport {
    QString anlageId;
    bool isConspicuous = false; // Auffällige Anlage?
    bool decliningPerformance = false;
    bool risingScrap = false;
    QString frequentFaultCode;
    int frequentFaultCount = 0;
    double avgOee = 0.0;
    double avgVerfuegbarkeit = 0.0;
    double avgLeistungsgrad = 0.0;
    double avgQualitaet = 0.0;
    double totalMtbfMin = 0.0;
    double totalMttrMin = 0.0;
    qint64 totalStoerSek = 0;
    QStringList summaryNotesDe;
    QStringList summaryNotesEn;
};

/**
 * @brief Ergebnis der Engpassanalyse einer Linie (Erweiterung 7)
 */
struct BottleneckEntry {
    QString anlageId;
    QString bezeichnung;
    int position = 0;
    double sollTaktzeitS = 0.0;
    double stoerzeitMin = 0.0;
    double causedStarvationDownstreamMin = 0.0; // Verursachter Materialmangel bei Nachfolger (pos + 1)
    double causedBlockageUpstreamMin = 0.0;     // Verursachter Rückstau bei Vorgänger (pos - 1)
    double bottleneckScore = 0.0;               // Kombinierter Engpass-Index
    bool isPrimaryBottleneck = false;
};

/**
 * @brief Pareto-Eintrag für Störcode-Auswertung (Erweiterung 1)
 */
struct StoercodeParetoItem {
    QString code;
    QString kategorie;
    QString beschreibung;
    int count = 0;
    double totalMinutes = 0.0;
    double cumulativePercent = 0.0;
};

class OeeCalculator {
public:
    /**
     * @brief Berechnet alle OEE-Kennzahlen, Zeitanteile, kumulierte Gutteile und MTBF/MTTR für ein Tagesprotokoll.
     */
    static OeeMetrics calculateDailyOee(const Schichtprotokoll& protokoll, double sollTaktzeitS);

    /**
     * @brief Berechnet die OEE-Kennzahlen separat für jede geplante Schicht (Früh, Spät, Nacht) (Eigene Erweiterung).
     */
    static QList<OeeMetrics> calculatePerShiftOee(const Schichtprotokoll& protokoll, double sollTaktzeitS);

    /**
     * @brief Berechnet den 14-Tage-Trend und erkennt automatisch auffällige Anlagen (Erweiterung 6).
     */
    static QPair<QList<OeeMetrics>, AnomalyReport> analyze14DaysTrend(
        const Anlage& anlage,
        const QList<std::shared_ptr<Schichtprotokoll>>& protokolle
    );

    /**
     * @brief Führt eine Engpassanalyse über alle Anlagen einer Linie für ein gegebenes Datum (oder alle Tage) durch (Erweiterung 7).
     */
    static QList<BottleneckEntry> analyzeLineBottlenecks(
        const QString& linieId,
        DataRepository& repo,
        const QDate& optionalSingleDate = QDate()
    );

    /**
     * @brief Berechnet eine Pareto-Statistik aller Störcodes über die geladenen/verfügbaren Protokolle (Erweiterung 1).
     */
    static QList<StoercodeParetoItem> calculateStoercodePareto(
        DataRepository& repo,
        const QString& filterLinieId = QString()
    );
};

} // namespace RoboCell
