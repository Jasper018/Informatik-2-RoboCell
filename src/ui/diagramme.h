#ifndef DIAGRAMME_H
#define DIAGRAMME_H

#include <QList>
#include <QRectF>
#include <QWidget>
#include <memory>
#include "logic/oeeberechnung.h"
#include "model/datentypen.h"

// Zeitleiste der Betriebszustaende fuer alle drei Schichten eines Tages
class ZustandsLeisteWidget : public QWidget {
    Q_OBJECT

public:
    explicit ZustandsLeisteWidget(QWidget* parent = nullptr);

    void setProtokoll(std::shared_ptr<Schichtprotokoll> protokoll);
    QSize sizeHint() const override { return QSize(680, 235); }

protected:
    void paintEvent(QPaintEvent* ereignis) override;
    void mouseMoveEvent(QMouseEvent* ereignis) override;

private:
    struct BalkenInfo {
        QRectF rechteck;
        QString schichtTitel;
        ZustandEintrag zustand;
    };

    std::shared_ptr<Schichtprotokoll> m_protokoll;
    QList<BalkenInfo> m_balken;
};

// Kurvendiagramm fuer kumulierte Gutteile und Ausschuss ueber die Zeit
class StueckzahlDiagrammWidget : public QWidget {
    Q_OBJECT

public:
    explicit StueckzahlDiagrammWidget(QWidget* parent = nullptr);

    void setVerlauf(const QList<Verlaufspunkt>& punkte);
    QSize sizeHint() const override { return QSize(680, 230); }

protected:
    void paintEvent(QPaintEvent* ereignis) override;
    void mouseMoveEvent(QMouseEvent* ereignis) override;

private:
    QList<Verlaufspunkt> m_punkte;
};

// 14-Tage-Trenddiagramm fuer V, L, Q und OEE
class TrendDiagrammWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrendDiagrammWidget(QWidget* parent = nullptr);

    void setTage(const QList<OeeKennzahlen>& tage);
    QSize sizeHint() const override { return QSize(680, 240); }

protected:
    void paintEvent(QPaintEvent* ereignis) override;
    void mouseMoveEvent(QMouseEvent* ereignis) override;

private:
    struct TagPunkt {
        double x = 0.0;
        OeeKennzahlen daten;
    };

    QList<OeeKennzahlen> m_tage;
    QList<TagPunkt> m_treffer;
};

#endif // DIAGRAMME_H
