#include "ui/CustomCharts.h"
#include "core/LanguageManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <cmath>
#include <algorithm>

namespace RoboCell {

QColor colorForZustandTyp(ZustandTyp typ) {
    switch (typ) {
        case ZustandTyp::Produktion:     return QColor("#16a34a"); // Grün
        case ZustandTyp::Ruesten:        return QColor("#2563eb"); // Blau
        case ZustandTyp::Stoerung:       return QColor("#dc2626"); // Rot
        case ZustandTyp::Materialmangel: return QColor("#f59e0b"); // Orange/Gelb
        case ZustandTyp::Rueckstau:      return QColor("#9333ea"); // Violett
        case ZustandTyp::Wartung:        return QColor("#0d9488"); // Türkis
        case ZustandTyp::Pause:          return QColor("#94a3b8"); // Grau
        case ZustandTyp::Unbekannt:      return QColor("#475569"); // Dunkelgrau
    }
    return QColor("#64748b");
}

QString displayNameForZustandTyp(ZustandTyp typ) {
    switch (typ) {
        case ZustandTyp::Produktion:     return trBi("Produktion", "Production");
        case ZustandTyp::Ruesten:        return trBi("Rüsten", "Setup");
        case ZustandTyp::Stoerung:       return trBi("Störung", "Fault");
        case ZustandTyp::Materialmangel: return trBi("Materialmangel", "Starvation");
        case ZustandTyp::Rueckstau:      return trBi("Rückstau", "Blockage");
        case ZustandTyp::Wartung:        return trBi("Wartung", "Maintenance");
        case ZustandTyp::Pause:          return trBi("Pause", "Break");
        case ZustandTyp::Unbekannt:      return trBi("Unbekannt / Stillstand", "Unknown / Downtime");
    }
    return "-";
}

// ============================================================================
// StateTimelineWidget
// ============================================================================

StateTimelineWidget::StateTimelineWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(215);
    setMouseTracking(true);
}

void StateTimelineWidget::setProtokoll(std::shared_ptr<Schichtprotokoll> protokoll) {
    m_protokoll = std::move(protokoll);
    update();
}

void StateTimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), QColor("#ffffff"));
    p.setPen(QColor("#cbd5e1"));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    m_hitBlocks.clear();

    // Titel + Legende oben
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(14, 22, trBi("Zustandsverlauf je Schicht (Zeitleiste – Maus über Balken für Details)",
                            "State Timeline per Shift (Hover over bars for details)"));

    // Legende zeichnen
    const QList<ZustandTyp> legendTypes = {
        ZustandTyp::Produktion, ZustandTyp::Ruesten, ZustandTyp::Stoerung,
        ZustandTyp::Materialmangel, ZustandTyp::Rueckstau, ZustandTyp::Wartung, ZustandTyp::Pause
    };
    QFont smallFont = font();
    smallFont.setPointSize(8);
    p.setFont(smallFont);

    int lx = 14;
    int ly = 32;
    for (ZustandTyp zt : legendTypes) {
        const QString lbl = displayNameForZustandTyp(zt);
        p.setBrush(colorForZustandTyp(zt));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(lx, ly, 12, 12, 2, 2);
        p.setPen(QColor("#334155"));
        const int tw = p.fontMetrics().horizontalAdvance(lbl);
        p.drawText(lx + 16, ly + 10, lbl);
        lx += 16 + tw + 14;
    }

    if (!m_protokoll || !m_protokoll->loadSuccess) {
        p.setPen(QColor("#dc2626"));
        p.setFont(titleFont);
        const QString msg = m_protokoll
            ? m_protokoll->errorMessage
            : trBi("Kein Schichtprotokoll geladen.", "No shift log loaded.");
        p.drawText(rect().adjusted(20, 50, -20, -20), Qt::AlignCenter | Qt::TextWordWrap, msg);
        return;
    }

    const int leftMargin = 115;
    const int rightMargin = 20;
    const int topStart = 58;
    const int barHeight = 32;
    const int rowSpacing = 46;
    const int availW = std::max(100, width() - leftMargin - rightMargin);

    for (int i = 0; i < m_protokoll->schichten.size(); ++i) {
        const Schicht& s = m_protokoll->schichten[i];
        const int y = topStart + i * rowSpacing;

        // Schichtname links
        QString sTitle = s.name;
        if (s.name == "frueh") sTitle = trBi("Früh (06–14h)", "Early (06–14h)");
        else if (s.name == "spaet") sTitle = trBi("Spät (14–22h)", "Late (14–22h)");
        else if (s.name == "nacht") sTitle = trBi("Nacht (22–06h)", "Night (22–06h)");

        QFont rowFont = font();
        rowFont.setBold(true);
        rowFont.setPointSize(9);
        p.setFont(rowFont);
        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(12, y, leftMargin - 16, barHeight), Qt::AlignVCenter | Qt::AlignLeft, sTitle);

        const QRectF barBg(leftMargin, y, availW, barHeight);
        p.setBrush(QColor("#f1f5f9"));
        p.setPen(QColor("#cbd5e1"));
        p.drawRoundedRect(barBg, 4, 4);

        if (!s.geplant || s.zustaende.isEmpty()) {
            p.setPen(QColor("#64748b"));
            p.setFont(smallFont);
            p.drawText(barBg, Qt::AlignCenter,
                       s.geplant ? trBi("Keine Zustandseinträge", "No state entries")
                                 : trBi("Schicht nicht geplant (geplant=\"nein\")", "Shift not planned (geplant=\"nein\")"));
            continue;
        }

        const qint64 shiftTotalSec = std::max<qint64>(1, s.beginn.secsTo(s.ende));
        for (const auto& z : s.zustaende) {
            if (!z.beginn.isValid() || !z.ende.isValid() || z.beginn >= z.ende) continue;
            const qint64 offsetSec = std::clamp<qint64>(s.beginn.secsTo(z.beginn), 0, shiftTotalSec);
            const qint64 endSec = std::clamp<qint64>(s.beginn.secsTo(z.ende), offsetSec, shiftTotalSec);
            const double x0 = leftMargin + (static_cast<double>(offsetSec) / shiftTotalSec) * availW;
            const double x1 = leftMargin + (static_cast<double>(endSec) / shiftTotalSec) * availW;
            const double w = std::max(1.5, x1 - x0);

            QRectF zRect(x0, y + 1, w, barHeight - 2);
            p.setBrush(colorForZustandTyp(z.typ));
            p.setPen(Qt::NoPen);
            p.drawRect(zRect);

            if (!z.isPlausible) {
                // Schraffur/Rahmen bei Datenwarnung
                p.setPen(QPen(QColor("#facc15"), 2));
                p.drawRect(zRect.adjusted(1, 1, -1, -1));
            }

            HitBlock hb;
            hb.rect = zRect;
            hb.schichtName = sTitle;
            hb.zustand = z;
            m_hitBlocks.append(hb);
        }

        // Stundenmarkierungen (0h .. 8h der Schicht)
        p.setPen(QColor("#64748b"));
        p.setFont(smallFont);
        for (int h = 0; h <= 8; h += 2) {
            const double hx = leftMargin + (h / 8.0) * availW;
            p.drawLine(QPointF(hx, y + barHeight), QPointF(hx, y + barHeight + 3));
            if (s.beginn.isValid()) {
                const QString tStr = s.beginn.addSecs(h * 3600).toString("HH:mm");
                p.drawText(QRectF(hx - 18, y + barHeight + 2, 36, 12), Qt::AlignCenter, tStr);
            }
        }
    }
}

void StateTimelineWidget::mouseMoveEvent(QMouseEvent* event) {
    for (const auto& hb : m_hitBlocks) {
        if (hb.rect.contains(event->pos())) {
            const auto& z = hb.zustand;
            const double minDur = z.dauerSekunden() / 60.0;
            QStringList lines;
            lines << QString("<b>%1 – %2</b>").arg(hb.schichtName, displayNameForZustandTyp(z.typ));
            lines << trBi("Zeitraum: %1 bis %2 (%3 Min.)", "Period: %1 to %2 (%3 min)")
                         .arg(z.beginn.toString("HH:mm:ss"), z.ende.toString("HH:mm:ss"))
                         .arg(minDur, 0, 'f', 1);
            if (!z.produktId.isEmpty()) {
                lines << trBi("Produkt: <b>%1</b>", "Product: <b>%1</b>").arg(z.produktId);
            }
            if (z.typ == ZustandTyp::Produktion) {
                lines << trBi("Gutteile: <b>%1</b> | Ausschuss: <b>%2</b>",
                              "Good parts: <b>%1</b> | Scrap: <b>%2</b>").arg(z.gut).arg(z.ausschuss);
            }
            if (!z.stoerCode.isEmpty()) {
                lines << trBi("Störcode: <b style='color:#dc2626;'>%1</b>",
                              "Fault code: <b style='color:#dc2626;'>%1</b>").arg(z.stoerCode);
            }
            if (!z.warningMessage.isEmpty()) {
                lines << "<span style='color:#d97706;'>⚠ " + z.warningMessage + "</span>";
            }
            QToolTip::showText(event->globalPosition().toPoint(), lines.join("<br>"), this);
            return;
        }
    }
    QToolTip::hideText();
}

// ============================================================================
// CumulativePartsChartWidget
// ============================================================================

CumulativePartsChartWidget::CumulativePartsChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(210);
    setMouseTracking(true);
}

void CumulativePartsChartWidget::setPoints(const QList<CumulativePartPoint>& points) {
    m_points = points;
    update();
}

void CumulativePartsChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), QColor("#ffffff"));
    p.setPen(QColor("#cbd5e1"));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(14, 22, trBi("Kumulierte Stückzahlen über die Zeit (Gutteile & Ausschuss)",
                            "Cumulative Part Count Over Time (Good Parts & Scrap)"));

    if (m_points.size() < 2) {
        p.setPen(QColor("#64748b"));
        p.drawText(rect(), Qt::AlignCenter,
                   trBi("Keine Produktionsdaten für diesen Tag vorhanden.",
                        "No production data available for this day."));
        return;
    }

    const int left = 58, right = 24, top = 38, bottom = 30;
    const QRectF plotRect(left, top, std::max(80, width() - left - right), std::max(60, height() - top - bottom));

    int maxVal = 10;
    for (const auto& pt : m_points) {
        maxVal = std::max(maxVal, pt.cumulativeGood);
    }
    maxVal = static_cast<int>(std::ceil(maxVal * 1.1));

    const QDateTime tStart = m_points.first().timestamp;
    const QDateTime tEnd = m_points.last().timestamp;
    const qint64 totalSecs = std::max<qint64>(1, tStart.secsTo(tEnd));

    // Gitterlinien & Y-Achse
    QFont axisFont = font();
    axisFont.setPointSize(8);
    p.setFont(axisFont);

    for (int i = 0; i <= 4; ++i) {
        const double ratio = i / 4.0;
        const double y = plotRect.bottom() - ratio * plotRect.height();
        const int val = static_cast<int>(std::round(ratio * maxVal));
        p.setPen(QPen(QColor("#e2e8f0"), 1, Qt::DashLine));
        p.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
        p.setPen(QColor("#475569"));
        p.drawText(QRectF(4, y - 8, left - 10, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
    }

    // X-Achse Zeitlabels
    for (int i = 0; i <= 4; ++i) {
        const double ratio = i / 4.0;
        const double x = plotRect.left() + ratio * plotRect.width();
        const QDateTime t = tStart.addSecs(static_cast<qint64>(ratio * totalSecs));
        p.setPen(QColor("#475569"));
        p.drawText(QRectF(x - 24, plotRect.bottom() + 4, 48, 18), Qt::AlignCenter, t.toString("HH:mm"));
    }

    // Pfad für kumulierte Gutteile
    QPainterPath linePath;
    QPainterPath areaPath;

    for (int i = 0; i < m_points.size(); ++i) {
        const double xr = static_cast<double>(tStart.secsTo(m_points[i].timestamp)) / totalSecs;
        const double yr = static_cast<double>(m_points[i].cumulativeGood) / maxVal;
        const QPointF pt(plotRect.left() + xr * plotRect.width(),
                         plotRect.bottom() - yr * plotRect.height());
        if (i == 0) {
            linePath.moveTo(pt);
            areaPath.moveTo(plotRect.left(), plotRect.bottom());
            areaPath.lineTo(pt);
        } else {
            linePath.lineTo(pt);
            areaPath.lineTo(pt);
        }
    }
    areaPath.lineTo(plotRect.right(), plotRect.bottom());
    areaPath.closeSubpath();

    p.fillPath(areaPath, QColor(22, 163, 74, 38));
    p.setPen(QPen(QColor("#16a34a"), 2.4));
    p.drawPath(linePath);

    // Endwert-Label
    const auto& lastPt = m_points.last();
    p.setPen(QColor("#15803d"));
    p.setFont(titleFont);
    p.drawText(QRectF(plotRect.right() - 220, top - 22, 220, 20), Qt::AlignRight | Qt::AlignVCenter,
               trBi("Gutteile: %1 | Ausschuss: %2", "Good Parts: %1 | Scrap: %2")
                   .arg(lastPt.cumulativeGood).arg(lastPt.cumulativeScrap));
}

void CumulativePartsChartWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_points.size() < 2) return;
    const int left = 58, right = 24;
    const double w = std::max(80, width() - left - right);
    const double relX = std::clamp((event->pos().x() - left) / w, 0.0, 1.0);
    const QDateTime tStart = m_points.first().timestamp;
    const qint64 totalSecs = std::max<qint64>(1, tStart.secsTo(m_points.last().timestamp));
    const QDateTime targetTime = tStart.addSecs(static_cast<qint64>(relX * totalSecs));

    const CumulativePartPoint* best = &m_points.first();
    for (const auto& pt : m_points) {
        if (pt.timestamp <= targetTime) best = &pt;
        else break;
    }

    QToolTip::showText(event->globalPosition().toPoint(),
        trBi("<b>Uhrzeit: %1</b><br>Kumulierte Gutteile: <b>%2</b><br>Kumulierter Ausschuss: <b>%3</b>",
             "<b>Time: %1</b><br>Cumulative Good Parts: <b>%2</b><br>Cumulative Scrap: <b>%3</b>")
            .arg(best->timestamp.toString("HH:mm:ss"))
            .arg(best->cumulativeGood)
            .arg(best->cumulativeScrap),
        this);
}

// ============================================================================
// Trend14DaysChartWidget
// ============================================================================

Trend14DaysChartWidget::Trend14DaysChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(230);
    setMouseTracking(true);
}

void Trend14DaysChartWidget::setDailyMetrics(const QList<OeeMetrics>& metrics) {
    m_metrics = metrics;
    update();
}

void Trend14DaysChartWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    p.fillRect(rect(), QColor("#ffffff"));
    p.setPen(QColor("#cbd5e1"));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    m_dayHits.clear();

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(14, 22, trBi("14-Tage-Trendanalyse: Verfügbarkeit (V), Leistungsgrad (L), Qualität (Q) & OEE",
                            "14-Day Trend Analysis: Availability (V), Performance (L), Quality (Q) & OEE"));

    // Legende rechts oben
    struct SeriesDef { QString name; QColor color; };
    const QList<SeriesDef> series = {
        {"OEE", QColor("#1e293b")},
        {trBi("Verfügbarkeit (V)", "Availability (V)"), QColor("#2563eb")},
        {trBi("Leistung (L)", "Performance (L)"), QColor("#d97706")},
        {trBi("Qualität (Q)", "Quality (Q)"), QColor("#16a34a")}
    };

    QFont smallFont = font();
    smallFont.setPointSize(8);
    p.setFont(smallFont);
    int lx = 14;
    int ly = 32;
    for (const auto& s : series) {
        p.setBrush(s.color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(lx, ly, 12, 12, 2, 2);
        p.setPen(QColor("#334155"));
        const int tw = p.fontMetrics().horizontalAdvance(s.name);
        p.drawText(lx + 16, ly + 10, s.name);
        lx += 16 + tw + 16;
    }

    if (m_metrics.isEmpty()) {
        p.drawText(rect(), Qt::AlignCenter, trBi("Keine 14-Tage-Daten verfügbar.", "No 14-day data available."));
        return;
    }

    const int left = 50, right = 24, top = 54, bottom = 32;
    const QRectF plotRect(left, top, std::max(80, width() - left - right), std::max(60, height() - top - bottom));

    // Y-Achse 0% .. 100%
    for (int pct = 0; pct <= 100; pct += 25) {
        const double y = plotRect.bottom() - (pct / 100.0) * plotRect.height();
        p.setPen(QPen(QColor("#e2e8f0"), 1, Qt::DashLine));
        p.drawLine(QPointF(plotRect.left(), y), QPointF(plotRect.right(), y));
        p.setPen(QColor("#475569"));
        p.drawText(QRectF(4, y - 8, left - 8, 16), Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(pct));
    }

    const int n = m_metrics.size();
    auto drawMetricLine = [&](auto getter, const QColor& col, double widthPx) {
        QPainterPath path;
        bool started = false;
        for (int i = 0; i < n; ++i) {
            if (!m_metrics[i].isValid) continue;
            const double x = plotRect.left() + (n > 1 ? (static_cast<double>(i) / (n - 1)) * plotRect.width() : plotRect.width() / 2.0);
            const double val = std::clamp(getter(m_metrics[i]), 0.0, 1.05);
            const double y = plotRect.bottom() - val * plotRect.height();
            if (!started) {
                path.moveTo(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
            p.setBrush(col);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(x, y), 3.2, 3.2);
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(col, widthPx));
        p.drawPath(path);
    };

    drawMetricLine([](const OeeMetrics& m) { return m.verfuegbarkeit; }, QColor("#2563eb"), 1.8);
    drawMetricLine([](const OeeMetrics& m) { return m.leistungsgrad; }, QColor("#d97706"), 1.8);
    drawMetricLine([](const OeeMetrics& m) { return m.qualitaetsrate; }, QColor("#16a34a"), 1.8);
    drawMetricLine([](const OeeMetrics& m) { return m.oee; }, QColor("#1e293b"), 2.8);

    // X-Achse Datumslabels
    p.setPen(QColor("#475569"));
    for (int i = 0; i < n; ++i) {
        const double x = plotRect.left() + (n > 1 ? (static_cast<double>(i) / (n - 1)) * plotRect.width() : plotRect.width() / 2.0);
        m_dayHits.append({x, m_metrics[i]});
        if (i % 2 == 0 || i == n - 1) {
            p.drawText(QRectF(x - 22, plotRect.bottom() + 5, 44, 18), Qt::AlignCenter, m_metrics[i].datum.toString("dd.MM."));
        }
    }
}

void Trend14DaysChartWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dayHits.isEmpty()) return;
    double bestDist = 1e9;
    const DayHit* best = nullptr;
    for (const auto& dh : m_dayHits) {
        const double d = std::abs(dh.x - event->pos().x());
        if (d < bestDist) {
            bestDist = d;
            best = &dh;
        }
    }
    if (best && bestDist < 30.0) {
        const auto& m = best->m;
        if (!m.isValid) {
            QToolTip::showText(event->globalPosition().toPoint(),
                trBi("<b>%1</b><br>Keine geplanten Schichten / Protokoll fehlt",
                     "<b>%1</b><br>No scheduled shifts / Log missing").arg(m.datum.toString("dd.MM.yyyy")), this);
        } else {
            QToolTip::showText(event->globalPosition().toPoint(),
                trBi("<b>%1</b><br>OEE: <b>%2%</b><br>Verfügbarkeit (V): %3%<br>Leistungsgrad (L): %4%<br>Qualitätsrate (Q): %5%<br>Gutteile: %6 | Ausschuss: %7",
                     "<b>%1</b><br>OEE: <b>%2%</b><br>Availability (V): %3%<br>Performance (L): %4%<br>Quality Rate (Q): %5%<br>Good Parts: %6 | Scrap: %7")
                    .arg(m.datum.toString("dd.MM.yyyy"))
                    .arg(m.oee * 100.0, 0, 'f', 1)
                    .arg(m.verfuegbarkeit * 100.0, 0, 'f', 1)
                    .arg(m.leistungsgrad * 100.0, 0, 'f', 1)
                    .arg(m.qualitaetsrate * 100.0, 0, 'f', 1)
                    .arg(m.gutteile)
                    .arg(m.ausschuss),
                this);
        }
    }
}

} // namespace RoboCell
