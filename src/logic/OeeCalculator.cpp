#include "logic/OeeCalculator.h"
#include "io/DataRepository.h"
#include <algorithm>
#include <cmath>

namespace RoboCell {

static void accumulateShiftIntoMetrics(const Schicht& schicht, OeeMetrics& m, int& runningGood, int& runningScrap) {
    if (!schicht.geplant) return;
    m.hasPlannedShifts = true;

    for (const auto& z : schicht.zustaende) {
        if (!z.beginn.isValid() || !z.ende.isValid() || z.beginn >= z.ende) {
            continue; // Ungültiges Zeitintervall (z.B. A007_2026-09-21.xml) überspringen
        }
        const qint64 dur = z.dauerSekunden();
        m.gesamtSchichtzeitSek += dur;
        m.sekundenJeZustand[z.typ] += dur;

        // Planbelegungszeit T_plan: alle Zustände in geplanten Schichten OHNE pause und wartung
        if (z.typ != ZustandTyp::Pause && z.typ != ZustandTyp::Wartung) {
            m.planbelegungszeitSek += dur;
        }

        if (z.typ == ZustandTyp::Produktion) {
            m.laufzeitSek += dur;
            const int g = std::max(0, z.gut);
            const int a = std::max(0, z.ausschuss);
            m.gutteile += g;
            m.ausschuss += a;
            runningGood += g;
            runningScrap += a;

            CumulativePartPoint pt;
            pt.timestamp = z.ende;
            pt.cumulativeGood = runningGood;
            pt.cumulativeScrap = runningScrap;
            m.cumulativePoints.append(pt);
        } else if (z.typ == ZustandTyp::Stoerung) {
            m.anzahlStoerungen += 1;
            m.stoerzeitSek += dur;
            if (!z.stoerCode.isEmpty()) {
                m.stoercodeCount[z.stoerCode] += 1;
                m.stoercodeSeconds[z.stoerCode] += dur;
            }
        }
    }
}

static void finalizeRatios(OeeMetrics& m, double sollTaktzeitS) {
    if (m.planbelegungszeitSek > 0) {
        m.verfuegbarkeit = static_cast<double>(m.laufzeitSek) / static_cast<double>(m.planbelegungszeitSek);
    } else {
        m.verfuegbarkeit = 0.0;
    }

    const int nGesamt = m.gutteile + m.ausschuss;
    if (m.laufzeitSek > 0 && sollTaktzeitS > 0.0) {
        m.leistungsgrad = (sollTaktzeitS * static_cast<double>(nGesamt)) / static_cast<double>(m.laufzeitSek);
    } else {
        m.leistungsgrad = 0.0;
    }

    if (nGesamt > 0) {
        m.qualitaetsrate = static_cast<double>(m.gutteile) / static_cast<double>(nGesamt);
    } else {
        m.qualitaetsrate = 0.0;
    }

    m.oee = m.verfuegbarkeit * m.leistungsgrad * m.qualitaetsrate;

    // MTBF & MTTR
    if (m.anzahlStoerungen > 0) {
        m.mttrMinuten = (static_cast<double>(m.stoerzeitSek) / 60.0) / static_cast<double>(m.anzahlStoerungen);
        m.mtbfMinuten = (static_cast<double>(m.laufzeitSek) / 60.0) / static_cast<double>(m.anzahlStoerungen);
    } else {
        m.mttrMinuten = 0.0;
        m.mtbfMinuten = static_cast<double>(m.laufzeitSek) / 60.0;
    }

    m.isValid = m.hasPlannedShifts && (m.planbelegungszeitSek > 0);
}

OeeMetrics OeeCalculator::calculateDailyOee(const Schichtprotokoll& protokoll, double sollTaktzeitS) {
    OeeMetrics m;
    m.datum = protokoll.datum;
    m.label = protokoll.datum.toString("yyyy-MM-dd");

    if (!protokoll.loadSuccess) {
        m.isValid = false;
        return m;
    }

    int runningGood = 0;
    int runningScrap = 0;

    // Startpunkt bei Schichtbeginn für schönere Verlaufskurve
    for (const auto& s : protokoll.schichten) {
        if (s.geplant && s.beginn.isValid()) {
            CumulativePartPoint startPt;
            startPt.timestamp = s.beginn;
            startPt.cumulativeGood = 0;
            startPt.cumulativeScrap = 0;
            m.cumulativePoints.append(startPt);
            break;
        }
    }

    for (const auto& s : protokoll.schichten) {
        accumulateShiftIntoMetrics(s, m, runningGood, runningScrap);
    }

    finalizeRatios(m, sollTaktzeitS);
    return m;
}

QList<OeeMetrics> OeeCalculator::calculatePerShiftOee(const Schichtprotokoll& protokoll, double sollTaktzeitS) {
    QList<OeeMetrics> result;
    if (!protokoll.loadSuccess) return result;

    for (const auto& s : protokoll.schichten) {
        OeeMetrics sm;
        sm.datum = protokoll.datum;
        sm.label = s.name;
        int rg = 0, rs = 0;
        accumulateShiftIntoMetrics(s, sm, rg, rs);
        finalizeRatios(sm, sollTaktzeitS);
        result.append(sm);
    }
    return result;
}

QPair<QList<OeeMetrics>, AnomalyReport> OeeCalculator::analyze14DaysTrend(
    const Anlage& anlage,
    const QList<std::shared_ptr<Schichtprotokoll>>& protokolle)
{
    QList<OeeMetrics> dailyList;
    AnomalyReport report;
    report.anlageId = anlage.anlageId;

    double sumOee = 0.0, sumV = 0.0, sumL = 0.0, sumQ = 0.0;
    int validDays = 0;
    qint64 totalLaufSek = 0;
    qint64 totalStoerSek = 0;
    int totalStoerCount = 0;
    QMap<QString, int> globalCodeCounts;

    QList<double> perfFirstHalf, perfSecondHalf;
    QList<double> scrapFirstHalf, scrapSecondHalf;

    const int nTotal = protokolle.size();
    for (int i = 0; i < nTotal; ++i) {
        const auto& proto = protokolle[i];
        if (!proto) continue;
        OeeMetrics dm = calculateDailyOee(*proto, anlage.sollTaktzeitS);
        dailyList.append(dm);

        if (!dm.isValid) continue;
        ++validDays;
        sumOee += dm.oee;
        sumV += dm.verfuegbarkeit;
        sumL += dm.leistungsgrad;
        sumQ += dm.qualitaetsrate;

        totalLaufSek += dm.laufzeitSek;
        totalStoerSek += dm.stoerzeitSek;
        totalStoerCount += dm.anzahlStoerungen;

        for (auto it = dm.stoercodeCount.cbegin(); it != dm.stoercodeCount.cend(); ++it) {
            globalCodeCounts[it.key()] += it.value();
        }

        const double scrapRate = (dm.gesamtteile() > 0)
            ? (static_cast<double>(dm.ausschuss) / static_cast<double>(dm.gesamtteile()))
            : 0.0;

        if (i < nTotal / 2) {
            perfFirstHalf.append(dm.leistungsgrad);
            scrapFirstHalf.append(scrapRate);
        } else {
            perfSecondHalf.append(dm.leistungsgrad);
            scrapSecondHalf.append(scrapRate);
        }
    }

    if (validDays > 0) {
        report.avgOee = sumOee / validDays;
        report.avgVerfuegbarkeit = sumV / validDays;
        report.avgLeistungsgrad = sumL / validDays;
        report.avgQualitaet = sumQ / validDays;
    }

    report.totalStoerSek = totalStoerSek;
    if (totalStoerCount > 0) {
        report.totalMttrMin = (static_cast<double>(totalStoerSek) / 60.0) / totalStoerCount;
        report.totalMtbfMin = (static_cast<double>(totalLaufSek) / 60.0) / totalStoerCount;
    }

    // 1. Prüfung auf sinkenden Leistungsgrad (Woche 2 vs. Woche 1)
    auto avgOf = [](const QList<double>& vals) {
        if (vals.isEmpty()) return 0.0;
        double s = 0.0;
        for (double v : vals) s += v;
        return s / vals.size();
    };

    const double l1 = avgOf(perfFirstHalf);
    const double l2 = avgOf(perfSecondHalf);
    if (!perfFirstHalf.isEmpty() && !perfSecondHalf.isEmpty() && (l1 - l2) > 0.035) {
        report.decliningPerformance = true;
        report.isConspicuous = true;
        report.summaryNotesDe.append(
            QString("Sinkender Leistungsgrad: Woche 1 Ø %1% → Woche 2 Ø %2%")
                .arg(l1 * 100.0, 0, 'f', 1).arg(l2 * 100.0, 0, 'f', 1));
        report.summaryNotesEn.append(
            QString("Declining performance: Week 1 avg %1% → Week 2 avg %2%")
                .arg(l1 * 100.0, 0, 'f', 1).arg(l2 * 100.0, 0, 'f', 1));
    }

    // 2. Prüfung auf steigenden Ausschuss
    const double s1 = avgOf(scrapFirstHalf);
    const double s2 = avgOf(scrapSecondHalf);
    if (!scrapFirstHalf.isEmpty() && !scrapSecondHalf.isEmpty() && (s2 - s1) > 0.012) {
        report.risingScrap = true;
        report.isConspicuous = true;
        report.summaryNotesDe.append(
            QString("Steigende Ausschussquote: Woche 1 Ø %1% → Woche 2 Ø %2%")
                .arg(s1 * 100.0, 0, 'f', 1).arg(s2 * 100.0, 0, 'f', 1));
        report.summaryNotesEn.append(
            QString("Rising scrap rate: Week 1 avg %1% → Week 2 avg %2%")
                .arg(s1 * 100.0, 0, 'f', 1).arg(s2 * 100.0, 0, 'f', 1));
    }

    // 3. Häufigster Störcode
    for (auto it = globalCodeCounts.cbegin(); it != globalCodeCounts.cend(); ++it) {
        if (it.value() > report.frequentFaultCount) {
            report.frequentFaultCount = it.value();
            report.frequentFaultCode = it.key();
        }
    }
    if (report.frequentFaultCount >= 22) {
        report.isConspicuous = true;
        report.summaryNotesDe.append(
            QString("Gehäufte Störung mit Code '%1' (%2x in 14 Tagen)")
                .arg(report.frequentFaultCode).arg(report.frequentFaultCount));
        report.summaryNotesEn.append(
            QString("Frequent fault code '%1' (%2x in 14 days)")
                .arg(report.frequentFaultCode).arg(report.frequentFaultCount));
    }

    // 4. Niedrige Gesamt-OEE (< 70%)
    if (validDays > 0 && report.avgOee < 0.70) {
        report.isConspicuous = true;
        report.summaryNotesDe.append(
            QString("Niedrige Durchschnitts-OEE (%1% < 70% Zielwert)").arg(report.avgOee * 100.0, 0, 'f', 1));
        report.summaryNotesEn.append(
            QString("Low average OEE (%1% < 70% target)").arg(report.avgOee * 100.0, 0, 'f', 1));
    }

    return qMakePair(dailyList, report);
}

QList<BottleneckEntry> OeeCalculator::analyzeLineBottlenecks(
    const QString& linieId,
    DataRepository& repo,
    const QDate& optionalSingleDate)
{
    QList<Anlage> lineAnlagen;
    for (const auto& a : repo.anlagen()) {
        if (a.linieId == linieId) lineAnlagen.append(a);
    }
    std::sort(lineAnlagen.begin(), lineAnlagen.end(), [](const Anlage& x, const Anlage& y) {
        return x.position < y.position;
    });

    QList<BottleneckEntry> entries;
    if (lineAnlagen.isEmpty()) return entries;

    // Für jede Anlage der Linie sammeln wir Störzeiten, Materialmangel- und Rückstauzeiten
    QMap<QString, qint64> stoerSekMap;
    QMap<QString, qint64> matMangelSekMap;
    QMap<QString, qint64> rueckstauSekMap;

    for (const auto& a : lineAnlagen) {
        const QList<QDate> dates = optionalSingleDate.isValid()
            ? QList<QDate>{optionalSingleDate}
            : repo.availableDatesForAnlage(a.anlageId);

        for (const QDate& d : dates) {
            auto proto = repo.getProtokollLazy(a.anlageId, d);
            if (!proto || !proto->loadSuccess) continue;
            OeeMetrics m = calculateDailyOee(*proto, a.sollTaktzeitS);
            stoerSekMap[a.anlageId] += m.sekundenJeZustand.value(ZustandTyp::Stoerung, 0);
            matMangelSekMap[a.anlageId] += m.sekundenJeZustand.value(ZustandTyp::Materialmangel, 0);
            rueckstauSekMap[a.anlageId] += m.sekundenJeZustand.value(ZustandTyp::Rueckstau, 0);
        }
    }

    int bestIdx = -1;
    double maxScore = -1.0;

    for (int i = 0; i < lineAnlagen.size(); ++i) {
        const auto& a = lineAnlagen[i];
        BottleneckEntry be;
        be.anlageId = a.anlageId;
        be.bezeichnung = a.bezeichnung;
        be.position = a.position;
        be.sollTaktzeitS = a.sollTaktzeitS;
        be.stoerzeitMin = static_cast<double>(stoerSekMap.value(a.anlageId, 0)) / 60.0;

        // Materialmangel der nachgelagerten Anlage (i + 1) wird durch diese Anlage verursacht
        if (i + 1 < lineAnlagen.size()) {
            const QString nextId = lineAnlagen[i + 1].anlageId;
            be.causedStarvationDownstreamMin = static_cast<double>(matMangelSekMap.value(nextId, 0)) / 60.0;
        }

        // Rückstau der vorgelagerten Anlage (i - 1) wird durch diese Anlage verursacht
        if (i - 1 >= 0) {
            const QString prevId = lineAnlagen[i - 1].anlageId;
            be.causedBlockageUpstreamMin = static_cast<double>(rueckstauSekMap.value(prevId, 0)) / 60.0;
        }

        // Kombinierter Engpass-Index aus Soll-Taktzeit, eigener Störzeit und verursachten Nachbarverlusten
        be.bottleneckScore = (be.sollTaktzeitS * 1.5)
                           + (be.stoerzeitMin * 0.6)
                           + ((be.causedStarvationDownstreamMin + be.causedBlockageUpstreamMin) * 0.8);

        if (be.bottleneckScore > maxScore) {
            maxScore = be.bottleneckScore;
            bestIdx = i;
        }
        entries.append(be);
    }

    if (bestIdx >= 0 && bestIdx < entries.size()) {
        entries[bestIdx].isPrimaryBottleneck = true;
    }

    return entries;
}

QList<StoercodeParetoItem> OeeCalculator::calculateStoercodePareto(
    DataRepository& repo,
    const QString& filterLinieId)
{
    QMap<QString, int> counts;
    QMap<QString, qint64> seconds;

    for (const auto& a : repo.anlagen()) {
        if (!filterLinieId.isEmpty() && a.linieId != filterLinieId) continue;
        const QList<QDate> dates = repo.availableDatesForAnlage(a.anlageId);
        for (const QDate& d : dates) {
            auto proto = repo.getProtokollLazy(a.anlageId, d);
            if (!proto || !proto->loadSuccess) continue;
            for (const auto& s : proto->schichten) {
                if (!s.geplant) continue;
                for (const auto& z : s.zustaende) {
                    if (z.typ == ZustandTyp::Stoerung && !z.stoerCode.isEmpty()) {
                        counts[z.stoerCode] += 1;
                        seconds[z.stoerCode] += z.dauerSekunden();
                    }
                }
            }
        }
    }

    QList<StoercodeParetoItem> items;
    double totalMinutesAll = 0.0;

    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        StoercodeParetoItem item;
        item.code = it.key();
        item.count = it.value();
        item.totalMinutes = static_cast<double>(seconds.value(it.key(), 0)) / 60.0;
        totalMinutesAll += item.totalMinutes;

        if (const Stoercode* sc = repo.findStoercode(item.code)) {
            item.kategorie = sc->kategorie;
            item.beschreibung = sc->beschreibung;
        } else {
            item.kategorie = "unbekannt";
            item.beschreibung = "Nicht in stoercodes.csv hinterlegt";
        }
        items.append(item);
    }

    // Sortieren absteigend nach Gesamtdauer (Minuten)
    std::sort(items.begin(), items.end(), [](const StoercodeParetoItem& a, const StoercodeParetoItem& b) {
        if (std::abs(a.totalMinutes - b.totalMinutes) > 1e-6)
            return a.totalMinutes > b.totalMinutes;
        return a.count > b.count;
    });

    double runningMinutes = 0.0;
    for (auto& item : items) {
        runningMinutes += item.totalMinutes;
        item.cumulativePercent = (totalMinutesAll > 0.0)
            ? (runningMinutes / totalMinutesAll) * 100.0
            : 0.0;
    }

    return items;
}

} // namespace RoboCell
