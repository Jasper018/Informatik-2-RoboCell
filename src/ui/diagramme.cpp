#include "diagramme.h"
#include "core/farben.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <algorithm>
#include <cmath>

static QString schichtAnzeigeName(const QString& roh)
{
    if (roh == "frueh") return QCoreApplication::translate("Diagramme", "Früh (06-14 Uhr)");
    if (roh == "spaet") return QCoreApplication::translate("Diagramme", "Spät (14-22 Uhr)");
    if (roh == "nacht") return QCoreApplication::translate("Diagramme", "Nacht (22-06 Uhr)");
    return roh;
}

// ===========================================================================
// ZustandsLeisteWidget
// ===========================================================================
ZustandsLeisteWidget::ZustandsLeisteWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(215);
    setMouseTracking(true);
}

void ZustandsLeisteWidget::setProtokoll(std::shared_ptr<Schichtprotokoll> protokoll)
{
    m_protokoll = protokoll;
    update();
}

void ZustandsLeisteWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), Farben::hintergrund());
    p.setPen(Farben::rahmen());
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    m_balken.clear();

    QFont titelSchrift = font();
    titelSchrift.setBold(true);
    p.setFont(titelSchrift);
    p.setPen(Farben::text());
    p.drawText(12, 22, tr("Zustandsverlauf je Schicht (Zeitleiste)"));

    // Legende mit automatischem Zeilenumbruch bei schmalen Fenstern
    QFont kleinSchrift = font();
    kleinSchrift.setPointSize(std::max(8, font().pointSize() - 1));
    p.setFont(kleinSchrift);

    const ZustandTyp legende[7] = {
        ZustandTyp::Produktion, ZustandTyp::Ruesten, ZustandTyp::Stoerung,
        ZustandTyp::Materialmangel, ZustandTyp::Rueckstau, ZustandTyp::Wartung, ZustandTyp::Pause
    };
    int lx = 12;
    int ly = 32;
    for (int i = 0; i < 7; i++) {
        const QString text = zustandName(legende[i]);
        const int breite = 18 + p.fontMetrics().horizontalAdvance(text) + 14;
        if (lx + breite > width() - 12 && lx > 12) {
            lx = 12;
            ly += 18;
        }
        p.setBrush(Farben::zustandFarbe(legende[i]));
        p.setPen(Farben::text());
        p.drawRect(lx, ly, 12, 12);
        p.setPen(Farben::text());
        p.drawText(lx + 16, ly + 10, text);
        lx += breite;
    }

    const int kopfUnten = ly + 22;

    if (!m_protokoll || !m_protokoll->geladen) {
        p.setFont(titelSchrift);
        p.setPen(Farben::fehlerText());
        const QString meldung = (!m_protokoll || m_protokoll->probleme.isEmpty())
            ? tr("Kein Schichtprotokoll geladen.")
            : m_protokoll->probleme.first().text();
        p.drawText(rect().adjusted(16, kopfUnten, -16, -12), Qt::AlignCenter | Qt::TextWordWrap, meldung);
        return;
    }

    const int links = 135;
    const int rechts = 18;
    const int balkenHoehe = 30;
    const int zeilenAbstand = 48;
    const int verfuegbar = std::max(100, width() - links - rechts);

    for (int i = 0; i < m_protokoll->schichten.size(); i++) {
        const Schicht& s = m_protokoll->schichten[i];
        const int y = kopfUnten + 4 + i * zeilenAbstand;
        const QString titel = schichtAnzeigeName(s.name);

        QFont zeilenSchrift = font();
        zeilenSchrift.setBold(true);
        p.setFont(zeilenSchrift);
        p.setPen(Farben::text());
        p.drawText(QRect(12, y, links - 18, balkenHoehe), Qt::AlignVCenter | Qt::AlignLeft, titel);

        const QRectF hintergrund(links, y, verfuegbar, balkenHoehe);
        p.setBrush(Farben::zebra());
        p.setPen(Farben::rahmen());
        p.drawRect(hintergrund);

        if (!s.geplant || s.zustaende.isEmpty()) {
            p.setFont(kleinSchrift);
            p.setPen(Farben::hinweisText());
            p.drawText(hintergrund, Qt::AlignCenter,
                       s.geplant ? tr("Keine Zustandseinträge") : tr("Nicht geplant"));
            continue;
        }

        const qint64 gesamtSek = std::max<qint64>(1, s.beginn.secsTo(s.ende));
        for (const ZustandEintrag& z : s.zustaende) {
            if (!z.beginn.isValid() || !z.ende.isValid() || z.beginn >= z.ende) continue;
            const qint64 startSek = std::clamp<qint64>(s.beginn.secsTo(z.beginn), 0, gesamtSek);
            const qint64 endeSek = std::clamp<qint64>(s.beginn.secsTo(z.ende), startSek, gesamtSek);
            const double x0 = links + double(startSek) / gesamtSek * verfuegbar;
            const double x1 = links + double(endeSek) / gesamtSek * verfuegbar;
            const double w = std::max(2.0, x1 - x0);

            const QRectF r(x0, y + 1, w, balkenHoehe - 2);
            p.setBrush(Farben::zustandFarbe(z.typ));
            p.setPen(Qt::NoPen);
            p.drawRect(r);

            if (!z.plausibel || z.typ == ZustandTyp::Unbekannt) {
                p.setPen(QPen(Farben::text(), 2));
                p.setBrush(Qt::NoBrush);
                p.drawRect(r.adjusted(1, 1, -1, -1));
            }

            BalkenInfo info;
            info.rechteck = r;
            info.schichtTitel = titel;
            info.zustand = z;
            m_balken.append(info);
        }

        // Zeitmarkierungen alle 2 Stunden
        p.setFont(kleinSchrift);
        p.setPen(Farben::hinweisText());
        for (int h = 0; h <= 8; h += 2) {
            const double hx = links + (h / 8.0) * verfuegbar;
            p.drawLine(QPointF(hx, y + balkenHoehe), QPointF(hx, y + balkenHoehe + 3));
            if (s.beginn.isValid()) {
                p.drawText(QRectF(hx - 20, y + balkenHoehe + 2, 40, 14), Qt::AlignCenter,
                           s.beginn.addSecs(h * 3600).toString("HH:mm"));
            }
        }
    }
}

void ZustandsLeisteWidget::mouseMoveEvent(QMouseEvent* ereignis)
{
    for (const BalkenInfo& b : m_balken) {
        if (!b.rechteck.contains(ereignis->pos())) continue;

        const ZustandEintrag& z = b.zustand;
        QStringList zeilen;
        zeilen.append(QString("%1 - %2").arg(b.schichtTitel, zustandName(z.typ)));
        zeilen.append(tr("Zeit: %1 bis %2 (%3 min)")
                          .arg(z.beginn.toString("HH:mm:ss"), z.ende.toString("HH:mm:ss"))
                          .arg(z.dauerSekunden() / 60.0, 0, 'f', 1));
        if (!z.produktId.isEmpty()) {
            zeilen.append(tr("Produkt: %1").arg(z.produktId));
        }
        if (z.typ == ZustandTyp::Produktion) {
            zeilen.append(tr("Gutteile: %1, Ausschuss: %2").arg(z.gut).arg(z.ausschuss));
        }
        if (!z.stoerCode.isEmpty()) {
            zeilen.append(tr("Störcode: %1").arg(z.stoerCode));
        }
        if (!z.plausibel || z.typ == ZustandTyp::Unbekannt) {
            zeilen.append(tr("Flag: Unplausibler Eintrag in diesem Intervall"));
        }
        QToolTip::showText(ereignis->globalPosition().toPoint(), zeilen.join("\n"), this);
        return;
    }
    QToolTip::hideText();
}

// ===========================================================================
// StueckzahlDiagrammWidget
// ===========================================================================
StueckzahlDiagrammWidget::StueckzahlDiagrammWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(210);
    setMouseTracking(true);
}

void StueckzahlDiagrammWidget::setVerlauf(const QList<Verlaufspunkt>& punkte)
{
    m_punkte = punkte;
    update();
}

void StueckzahlDiagrammWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), Farben::hintergrund());
    p.setPen(Farben::rahmen());
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    QFont titelSchrift = font();
    titelSchrift.setBold(true);
    p.setFont(titelSchrift);
    p.setPen(Farben::text());
    p.drawText(12, 22, tr("Kumulierte Stückzahlen über die Zeit"));

    if (m_punkte.size() < 2) {
        p.setFont(font());
        p.setPen(Farben::hinweisText());
        p.drawText(rect().adjusted(16, 32, -16, -12), Qt::AlignCenter,
                   tr("Keine Produktionsdaten für diesen Tag vorhanden."));
        return;
    }

    // Legende
    QFont kleinSchrift = font();
    kleinSchrift.setPointSize(std::max(8, font().pointSize() - 1));
    p.setFont(kleinSchrift);

    p.setPen(QPen(Farben::linieQ(), 3));
    p.drawLine(14, 38, 30, 38);
    p.setPen(Farben::text());
    const QString gutLbl = tr("Gutteile (gesamt: %1)").arg(m_punkte.last().gut);
    p.drawText(35, 42, gutLbl);

    const int x2 = 45 + p.fontMetrics().horizontalAdvance(gutLbl) + 18;
    p.setPen(QPen(Farben::linieAusschuss(), 2, Qt::DashLine));
    p.drawLine(x2, 38, x2 + 16, 38);
    p.setPen(Farben::text());
    p.drawText(x2 + 21, 42, tr("Ausschuss (gesamt: %1)").arg(m_punkte.last().ausschuss));

    const int links = 58, rechts = 24, oben = 54, unten = 32;
    const QRectF feld(links, oben, std::max(80, width() - links - rechts),
                      std::max(60, height() - oben - unten));

    int maxWert = 10;
    for (const Verlaufspunkt& pt : m_punkte) {
        maxWert = std::max(maxWert, pt.gut);
    }
    maxWert = int(std::ceil(maxWert * 1.1));

    const QDateTime start = m_punkte.first().zeit;
    const QDateTime ende = m_punkte.last().zeit;
    const qint64 gesamtSek = std::max<qint64>(1, start.secsTo(ende));

    // Gitter und Achsen
    for (int i = 0; i <= 4; i++) {
        const double anteil = i / 4.0;
        const double y = feld.bottom() - anteil * feld.height();
        p.setPen(QPen(Farben::rahmen(), 1, Qt::DotLine));
        p.drawLine(QPointF(feld.left(), y), QPointF(feld.right(), y));
        p.setPen(Farben::text());
        p.drawText(QRectF(4, y - 8, links - 10, 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(int(std::round(anteil * maxWert))));
    }
    for (int i = 0; i <= 4; i++) {
        const double anteil = i / 4.0;
        const double x = feld.left() + anteil * feld.width();
        p.setPen(Farben::text());
        p.drawText(QRectF(x - 24, feld.bottom() + 4, 48, 18), Qt::AlignCenter,
                   start.addSecs(qint64(anteil * gesamtSek)).toString("HH:mm"));
    }

    QPainterPath gutPfad;
    QPainterPath ausschussPfad;
    for (int i = 0; i < m_punkte.size(); i++) {
        const double xr = double(start.secsTo(m_punkte[i].zeit)) / gesamtSek;
        const double x = feld.left() + xr * feld.width();
        const double yGut = feld.bottom() - (double(m_punkte[i].gut) / maxWert) * feld.height();
        const double yAus = feld.bottom() - (double(m_punkte[i].ausschuss) / maxWert) * feld.height();
        if (i == 0) {
            gutPfad.moveTo(x, yGut);
            ausschussPfad.moveTo(x, yAus);
        } else {
            gutPfad.lineTo(x, yGut);
            ausschussPfad.lineTo(x, yAus);
        }
    }

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Farben::linieQ(), 2.5));
    p.drawPath(gutPfad);
    p.setPen(QPen(Farben::linieAusschuss(), 2.0, Qt::DashLine));
    p.drawPath(ausschussPfad);
}

void StueckzahlDiagrammWidget::mouseMoveEvent(QMouseEvent* ereignis)
{
    if (m_punkte.size() < 2) return;
    const int links = 58, rechts = 24;
    const double breite = std::max(80, width() - links - rechts);
    const double anteil = std::clamp((ereignis->pos().x() - links) / breite, 0.0, 1.0);
    const QDateTime start = m_punkte.first().zeit;
    const qint64 gesamtSek = std::max<qint64>(1, start.secsTo(m_punkte.last().zeit));
    const QDateTime zielZeit = start.addSecs(qint64(anteil * gesamtSek));

    const Verlaufspunkt* treffer = &m_punkte.first();
    for (const Verlaufspunkt& pt : m_punkte) {
        if (pt.zeit <= zielZeit) treffer = &pt;
        else break;
    }

    QToolTip::showText(ereignis->globalPosition().toPoint(),
        tr("Uhrzeit: %1\nKumulierte Gutteile: %2\nKumulierter Ausschuss: %3")
            .arg(treffer->zeit.toString("HH:mm:ss"))
            .arg(treffer->gut)
            .arg(treffer->ausschuss),
        this);
}

// ===========================================================================
// TrendDiagrammWidget
// ===========================================================================
TrendDiagrammWidget::TrendDiagrammWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(220);
    setMouseTracking(true);
}

void TrendDiagrammWidget::setTage(const QList<OeeKennzahlen>& tage)
{
    m_tage = tage;
    update();
}

void TrendDiagrammWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), Farben::hintergrund());
    p.setPen(Farben::rahmen());
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    m_treffer.clear();

    QFont titelSchrift = font();
    titelSchrift.setBold(true);
    p.setFont(titelSchrift);
    p.setPen(Farben::text());
    p.drawText(12, 22, tr("14-Tage-Trend: Verfügbarkeit (V), Leistungsgrad (L), Qualität (Q) und OEE"));

    struct Reihe { QString name; QColor farbe; Qt::PenStyle stil; };
    const Reihe reihen[4] = {
        {tr("OEE"), Farben::linieOee(), Qt::SolidLine},
        {tr("Verfügbarkeit (V)"), Farben::linieV(), Qt::SolidLine},
        {tr("Leistung (L)"), Farben::linieL(), Qt::DashLine},
        {tr("Qualität (Q)"), Farben::linieQ(), Qt::DotLine}
    };

    QFont kleinSchrift = font();
    kleinSchrift.setPointSize(std::max(8, font().pointSize() - 1));
    p.setFont(kleinSchrift);

    int lx = 12;
    int ly = 36;
    for (int i = 0; i < 4; i++) {
        const int breite = 24 + p.fontMetrics().horizontalAdvance(reihen[i].name) + 16;
        if (lx + breite > width() - 12 && lx > 12) {
            lx = 12;
            ly += 18;
        }
        p.setPen(QPen(reihen[i].farbe, 3, reihen[i].stil));
        p.drawLine(lx, ly, lx + 16, ly);
        p.setPen(Farben::text());
        p.drawText(lx + 22, ly + 4, reihen[i].name);
        lx += breite;
    }

    if (m_tage.isEmpty()) {
        p.setPen(Farben::hinweisText());
        p.drawText(rect().adjusted(16, ly + 16, -16, -12), Qt::AlignCenter,
                   tr("Keine 14-Tage-Daten verfügbar."));
        return;
    }

    const int links = 52, rechts = 24, oben = ly + 18, unten = 32;
    const QRectF feld(links, oben, std::max(80, width() - links - rechts),
                      std::max(60, height() - oben - unten));

    for (int pct = 0; pct <= 100; pct += 25) {
        const double y = feld.bottom() - (pct / 100.0) * feld.height();
        p.setPen(QPen(Farben::rahmen(), 1, Qt::DotLine));
        p.drawLine(QPointF(feld.left(), y), QPointF(feld.right(), y));
        p.setPen(Farben::text());
        p.drawText(QRectF(4, y - 8, links - 8, 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1 %").arg(pct));
    }

    const int n = m_tage.size();
    for (int r = 0; r < 4; r++) {
        QPainterPath pfad;
        bool gestartet = false;
        for (int i = 0; i < n; i++) {
            if (!m_tage[i].gueltig) continue;
            const double x = feld.left() + (n > 1 ? double(i) / (n - 1) * feld.width() : feld.width() / 2.0);
            double wert = 0.0;
            if (r == 0) wert = m_tage[i].oee;
            else if (r == 1) wert = m_tage[i].verfuegbarkeit;
            else if (r == 2) wert = m_tage[i].leistungsgrad;
            else wert = m_tage[i].qualitaet;

            const double y = feld.bottom() - std::clamp(wert, 0.0, 1.0) * feld.height();
            if (!gestartet) {
                pfad.moveTo(x, y);
                gestartet = true;
            } else {
                pfad.lineTo(x, y);
            }
            p.setBrush(reihen[r].farbe);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(x, y), 3.0, 3.0);
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(reihen[r].farbe, r == 0 ? 2.6 : 1.8, reihen[r].stil));
        p.drawPath(pfad);
    }

    p.setPen(Farben::text());
    for (int i = 0; i < n; i++) {
        const double x = feld.left() + (n > 1 ? double(i) / (n - 1) * feld.width() : feld.width() / 2.0);
        m_treffer.append({x, m_tage[i]});
        if (i % 2 == 0 || i == n - 1) {
            p.drawText(QRectF(x - 22, feld.bottom() + 4, 44, 18), Qt::AlignCenter,
                       m_tage[i].datum.toString("dd.MM."));
        }
    }
}

void TrendDiagrammWidget::mouseMoveEvent(QMouseEvent* ereignis)
{
    if (m_treffer.isEmpty()) return;
    double besterAbstand = 1e9;
    const TagPunkt* bester = nullptr;
    for (const TagPunkt& tp : m_treffer) {
        const double abstand = std::abs(tp.x - ereignis->pos().x());
        if (abstand < besterAbstand) {
            besterAbstand = abstand;
            bester = &tp;
        }
    }
    if (!bester || besterAbstand > 28.0) return;

    const OeeKennzahlen& m = bester->daten;
    if (!m.gueltig) {
        QToolTip::showText(ereignis->globalPosition().toPoint(),
            tr("%1\nKeine geplanten Schichten oder Protokoll ungültig").arg(m.datum.toString("dd.MM.yyyy")),
            this);
    } else {
        QToolTip::showText(ereignis->globalPosition().toPoint(),
            tr("%1\nOEE: %2 %\nVerfügbarkeit (V): %3 %\nLeistungsgrad (L): %4 %\nQualitätsrate (Q): %5 %\nGutteile: %6, Ausschuss: %7")
                .arg(m.datum.toString("dd.MM.yyyy"))
                .arg(m.oee * 100.0, 0, 'f', 1)
                .arg(m.verfuegbarkeit * 100.0, 0, 'f', 1)
                .arg(m.leistungsgrad * 100.0, 0, 'f', 1)
                .arg(m.qualitaet * 100.0, 0, 'f', 1)
                .arg(m.gut)
                .arg(m.ausschuss),
            this);
    }
}
