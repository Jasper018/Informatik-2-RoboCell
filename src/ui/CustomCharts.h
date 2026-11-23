#pragma once

#include "model/Entities.h"
#include "logic/OeeCalculator.h"
#include <QWidget>
#include <QList>
#include <QRectF>
#include <memory>

namespace RoboCell {

/**
 * @brief Liefert die konsistente Darstellungfarbe für jeden Zustandstyp.
 */
QColor colorForZustandTyp(ZustandTyp typ);
QString displayNameForZustandTyp(ZustandTyp typ);

/**
 * @brief Farbige Gantt-Zeitleiste des Zustandsverlaufs über die 3 Schichten (Mindestanforderung 5).
 */
class StateTimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit StateTimelineWidget(QWidget* parent = nullptr);

    void setProtokoll(std::shared_ptr<Schichtprotokoll> protokoll);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    struct HitBlock {
        QRectF rect;
        QString schichtName;
        ZustandEintrag zustand;
    };

    std::shared_ptr<Schichtprotokoll> m_protokoll;
    QList<HitBlock> m_hitBlocks;
};

/**
 * @brief Liniendiagramm für die kumulierte Anzahl der Gutteile & Ausschuss über die Zeit (Mindestanforderung 5).
 */
class CumulativePartsChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit CumulativePartsChartWidget(QWidget* parent = nullptr);

    void setPoints(const QList<CumulativePartPoint>& points);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QList<CumulativePartPoint> m_points;
};

/**
 * @brief 14-Tage-Trenddiagramm für Verfügbarkeit (V), Leistungsgrad (L), Qualitätsrate (Q) und OEE (Erweiterung 6).
 */
class Trend14DaysChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit Trend14DaysChartWidget(QWidget* parent = nullptr);

    void setDailyMetrics(const QList<OeeMetrics>& metrics);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QList<OeeMetrics> m_metrics;
    struct DayHit {
        double x = 0.0;
        OeeMetrics m;
    };
    QList<DayHit> m_dayHits;
};

} // namespace RoboCell
