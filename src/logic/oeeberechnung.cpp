#include "oeeberechnung.h"
#include <algorithm>
#include <cmath>

// Schwellwerte der automatischen Trend-Erkennung
static const double LEISTUNG_ABFALL = 0.035;     // Leistungsgrad sinkt um mehr als 3,5 Prozentpunkte
static const double AUSSCHUSS_ANSTIEG = 0.012;   // Ausschussquote steigt um mehr als 1,2 Prozentpunkte
static const double OEE_UNTERGRENZE = 0.70;      // Mittlere OEE unter 70 %
static const int CODE_HAEUFIG = 22;              // Gleicher Stoercode mindestens so oft in 14 Tagen

void OeeBerechnung::schichtAddieren(const Schicht& schicht, OeeKennzahlen* k,
                                    int* summeGut, int* summeAusschuss)
{
    if (!schicht.geplant) return;
    k->hatGeplanteSchicht = true;

    for (const ZustandEintrag& z : schicht.zustaende) {
        const qint64 dauer = z.dauerSekunden();
        if (dauer <= 0) continue;   // ungueltiges Intervall wird nicht gewertet

        k->schichtzeitSek += dauer;
        k->zustandSek[z.typ] += dauer;
        if (z.typ != ZustandTyp::Pause && z.typ != ZustandTyp::Wartung) {
            k->planzeitSek += dauer;
        }

        if (z.typ == ZustandTyp::Produktion) {
            k->laufzeitSek += dauer;
            k->gut += z.gut;
            k->ausschuss += z.ausschuss;
            *summeGut += z.gut;
            *summeAusschuss += z.ausschuss;

            Verlaufspunkt punkt;
            punkt.zeit = z.ende;
            punkt.gut = *summeGut;
            punkt.ausschuss = *summeAusschuss;
            k->verlauf.append(punkt);
        }
        else if (z.typ == ZustandTyp::Stoerung) {
            k->anzahlStoerungen++;
            k->stoerzeitSek += dauer;
            if (!z.stoerCode.isEmpty()) {
                k->codeAnzahl[z.stoerCode] += 1;
                k->codeSek[z.stoerCode] += dauer;
            }
        }
    }
}

void OeeBerechnung::faktorenBerechnen(OeeKennzahlen* k, double sollTaktzeitS)
{
    // V = T_lauf / T_plan
    if (k->planzeitSek > 0) {
        k->verfuegbarkeit = double(k->laufzeitSek) / double(k->planzeitSek);
    }
    // L = t_soll * (n_gut + n_aus) / T_lauf
    if (k->laufzeitSek > 0 && sollTaktzeitS > 0.0) {
        k->leistungsgrad = sollTaktzeitS * k->gesamt() / double(k->laufzeitSek);
    }
    // Q = n_gut / (n_gut + n_aus)
    if (k->gesamt() > 0) {
        k->qualitaet = double(k->gut) / double(k->gesamt());
    }
    k->oee = k->verfuegbarkeit * k->leistungsgrad * k->qualitaet;

    // MTBF und MTTR
    if (k->anzahlStoerungen > 0) {
        k->mttrMin = (k->stoerzeitSek / 60.0) / k->anzahlStoerungen;
        k->mtbfMin = (k->laufzeitSek / 60.0) / k->anzahlStoerungen;
    } else {
        k->mtbfMin = k->laufzeitSek / 60.0;
    }

    k->gueltig = k->hatGeplanteSchicht && k->planzeitSek > 0;
}

OeeKennzahlen OeeBerechnung::tag(const Schichtprotokoll& protokoll, double sollTaktzeitS)
{
    OeeKennzahlen k;
    k.anlageId = protokoll.anlageId;
    k.datum = protokoll.datum;
    k.bezeichnung = protokoll.datum.toString("yyyy-MM-dd");
    if (!protokoll.geladen) return k;

    int summeGut = 0;
    int summeAusschuss = 0;

    // Die Kurve beginnt bei 0 zu Beginn der ersten geplanten Schicht
    for (const Schicht& s : protokoll.schichten) {
        if (s.geplant && s.beginn.isValid()) {
            Verlaufspunkt start;
            start.zeit = s.beginn;
            k.verlauf.append(start);
            break;
        }
    }
    for (const Schicht& s : protokoll.schichten) {
        schichtAddieren(s, &k, &summeGut, &summeAusschuss);
    }
    faktorenBerechnen(&k, sollTaktzeitS);
    return k;
}

QList<OeeKennzahlen> OeeBerechnung::schichten(const Schichtprotokoll& protokoll, double sollTaktzeitS)
{
    QList<OeeKennzahlen> liste;
    if (!protokoll.geladen) return liste;

    for (const Schicht& s : protokoll.schichten) {
        OeeKennzahlen k;
        k.anlageId = protokoll.anlageId;
        k.datum = protokoll.datum;
        k.bezeichnung = s.name;
        int summeGut = 0;
        int summeAusschuss = 0;
        schichtAddieren(s, &k, &summeGut, &summeAusschuss);
        faktorenBerechnen(&k, sollTaktzeitS);
        liste.append(k);
    }
    return liste;
}

static double mittelwert(const QList<double>& werte)
{
    if (werte.isEmpty()) return 0.0;
    double summe = 0.0;
    for (double w : werte) summe += w;
    return summe / werte.size();
}

Trendbericht OeeBerechnung::trend(const Anlage& anlage, const QList<OeeKennzahlen>& tage)
{
    Trendbericht b;
    b.anlageId = anlage.anlageId;

    double summeOee = 0, summeV = 0, summeL = 0, summeQ = 0;
    qint64 laufSek = 0;
    int stoerungen = 0;
    QMap<QString, int> codes;
    QList<double> leistungErsteHaelfte, leistungZweiteHaelfte;
    QList<double> ausschussErsteHaelfte, ausschussZweiteHaelfte;

    for (int i = 0; i < tage.size(); i++) {
        const OeeKennzahlen& k = tage[i];
        if (!k.gueltig) continue;

        b.gueltigeTage++;
        summeOee += k.oee;
        summeV += k.verfuegbarkeit;
        summeL += k.leistungsgrad;
        summeQ += k.qualitaet;
        laufSek += k.laufzeitSek;
        b.stoerzeitSek += k.stoerzeitSek;
        stoerungen += k.anzahlStoerungen;
        for (auto it = k.codeAnzahl.begin(); it != k.codeAnzahl.end(); ++it) {
            codes[it.key()] += it.value();
        }

        const double ausschussQuote = (k.gesamt() > 0) ? double(k.ausschuss) / k.gesamt() : 0.0;
        if (i < tage.size() / 2) {
            leistungErsteHaelfte.append(k.leistungsgrad);
            ausschussErsteHaelfte.append(ausschussQuote);
        } else {
            leistungZweiteHaelfte.append(k.leistungsgrad);
            ausschussZweiteHaelfte.append(ausschussQuote);
        }
    }

    if (b.gueltigeTage > 0) {
        b.mittelOee = summeOee / b.gueltigeTage;
        b.mittelVerfuegbarkeit = summeV / b.gueltigeTage;
        b.mittelLeistung = summeL / b.gueltigeTage;
        b.mittelQualitaet = summeQ / b.gueltigeTage;
    }
    if (stoerungen > 0) {
        b.mttrMin = (b.stoerzeitSek / 60.0) / stoerungen;
        b.mtbfMin = (laufSek / 60.0) / stoerungen;
    }

    // 1. Sinkender Leistungsgrad (zweite Woche gegen erste Woche)
    b.leistungWoche1 = mittelwert(leistungErsteHaelfte);
    b.leistungWoche2 = mittelwert(leistungZweiteHaelfte);
    if (!leistungErsteHaelfte.isEmpty() && !leistungZweiteHaelfte.isEmpty()
        && (b.leistungWoche1 - b.leistungWoche2) > LEISTUNG_ABFALL) {
        b.sinkendeLeistung = true;
    }

    // 2. Steigender Ausschuss
    b.ausschussWoche1 = mittelwert(ausschussErsteHaelfte);
    b.ausschussWoche2 = mittelwert(ausschussZweiteHaelfte);
    if (!ausschussErsteHaelfte.isEmpty() && !ausschussZweiteHaelfte.isEmpty()
        && (b.ausschussWoche2 - b.ausschussWoche1) > AUSSCHUSS_ANSTIEG) {
        b.steigenderAusschuss = true;
    }

    // 3. Gehaeufte Stoerungen mit demselben Code
    for (auto it = codes.begin(); it != codes.end(); ++it) {
        if (it.value() > b.haeufigsterCodeAnzahl) {
            b.haeufigsterCodeAnzahl = it.value();
            b.haeufigsterCode = it.key();
        }
    }
    b.haeufigerCode = (b.haeufigsterCodeAnzahl >= CODE_HAEUFIG);

    // 4. Niedrige mittlere OEE
    b.niedrigeOee = (b.gueltigeTage > 0 && b.mittelOee < OEE_UNTERGRENZE);

    b.auffaellig = b.sinkendeLeistung || b.steigenderAusschuss || b.haeufigerCode || b.niedrigeOee;
    return b;
}

QStringList Trendbericht::hinweise() const
{
    QStringList liste;
    if (sinkendeLeistung) {
        liste.append(tr("Leistungsgrad sinkt von %1 % (Woche 1) auf %2 % (Woche 2)")
                         .arg(leistungWoche1 * 100.0, 0, 'f', 1).arg(leistungWoche2 * 100.0, 0, 'f', 1));
    }
    if (steigenderAusschuss) {
        liste.append(tr("Ausschussquote steigt von %1 % (Woche 1) auf %2 % (Woche 2)")
                         .arg(ausschussWoche1 * 100.0, 0, 'f', 1).arg(ausschussWoche2 * 100.0, 0, 'f', 1));
    }
    if (haeufigerCode) {
        liste.append(tr("Gehäufte Störung mit Code %1 (%2 mal in 14 Tagen)")
                         .arg(haeufigsterCode).arg(haeufigsterCodeAnzahl));
    }
    if (niedrigeOee) {
        liste.append(tr("Niedrige mittlere OEE (%1 % unter 70 %)").arg(mittelOee * 100.0, 0, 'f', 1));
    }
    return liste;
}

QList<EngpassEintrag> OeeBerechnung::engpass(const QList<Anlage>& stationen,
                                             const QList<OeeKennzahlen>& kennzahlen)
{
    QList<EngpassEintrag> liste;
    int besterIndex = -1;
    double besterScore = -1.0;

    for (int i = 0; i < stationen.size(); i++) {
        EngpassEintrag e;
        e.anlageId = stationen[i].anlageId;
        e.bezeichnung = stationen[i].bezeichnung;
        e.position = stationen[i].position;
        e.sollTaktzeitS = stationen[i].sollTaktzeitS;
        e.stoerzeitMin = kennzahlen[i].zustandSek.value(ZustandTyp::Stoerung, 0) / 60.0;

        // Materialmangel der nachfolgenden Anlage geht auf diese Anlage zurueck
        if (i + 1 < stationen.size()) {
            e.verursachterMangelMin = kennzahlen[i + 1].zustandSek.value(ZustandTyp::Materialmangel, 0) / 60.0;
        }
        // Rueckstau der vorgelagerten Anlage geht auf diese Anlage zurueck
        if (i > 0) {
            e.verursachterRueckstauMin = kennzahlen[i - 1].zustandSek.value(ZustandTyp::Rueckstau, 0) / 60.0;
        }

        e.score = e.sollTaktzeitS * 1.5 + e.stoerzeitMin * 0.6
                  + (e.verursachterMangelMin + e.verursachterRueckstauMin) * 0.8;
        if (e.score > besterScore) {
            besterScore = e.score;
            besterIndex = i;
        }
        liste.append(e);
    }
    if (besterIndex >= 0) liste[besterIndex].hauptEngpass = true;
    return liste;
}

QList<ParetoEintrag> OeeBerechnung::pareto(const QList<OeeKennzahlen>& tage)
{
    QMap<QString, int> anzahl;
    QMap<QString, qint64> sekunden;
    for (const OeeKennzahlen& k : tage) {
        for (auto it = k.codeAnzahl.begin(); it != k.codeAnzahl.end(); ++it) {
            anzahl[it.key()] += it.value();
        }
        for (auto it = k.codeSek.begin(); it != k.codeSek.end(); ++it) {
            sekunden[it.key()] += it.value();
        }
    }

    QList<ParetoEintrag> liste;
    double gesamtMinuten = 0.0;
    for (auto it = anzahl.begin(); it != anzahl.end(); ++it) {
        ParetoEintrag e;
        e.code = it.key();
        e.anzahl = it.value();
        e.minuten = sekunden.value(it.key(), 0) / 60.0;
        gesamtMinuten += e.minuten;
        liste.append(e);
    }

    // Absteigend nach Gesamtdauer sortieren
    std::sort(liste.begin(), liste.end(), [](const ParetoEintrag& a, const ParetoEintrag& b) {
        if (std::abs(a.minuten - b.minuten) > 1e-6) return a.minuten > b.minuten;
        return a.anzahl > b.anzahl;
    });

    double laufend = 0.0;
    for (ParetoEintrag& e : liste) {
        laufend += e.minuten;
        e.kumuliertProzent = (gesamtMinuten > 0.0) ? laufend / gesamtMinuten * 100.0 : 0.0;
    }
    return liste;
}
