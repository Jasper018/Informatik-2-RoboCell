#include "ui/LineGraphicsView.h"
#include "ui/CustomCharts.h"
#include "io/DataRepository.h"
#include "core/LanguageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QHeaderView>
#include <QSplitter>
#include <algorithm>

namespace RoboCell {

LineFlowWidget::LineFlowWidget(DataRepository* repo, QWidget* parent)
    : QWidget(parent), m_repo(repo)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Top Control Bar
    auto* ctrlLayout = new QHBoxLayout();
    m_lineTitleLabel = new QLabel(this);
    m_lineCombo = new QComboBox(this);

    m_dateTitleLabel = new QLabel(this);
    m_dateCombo = new QComboBox(this);

    m_modeTitleLabel = new QLabel(this);
    m_colorModeCombo = new QComboBox(this);

    m_sliderTitleLabel = new QLabel(this);
    m_timeSlider = new QSlider(Qt::Horizontal, this);
    m_timeSlider->setRange(0, 24 * 60 - 1); // 06:00 bis 05:59 Folgetag (1440 Minuten)
    m_timeSlider->setValue(180); // Default 09:00 Uhr
    m_timeSlider->setMinimumWidth(180);

    m_timeValueLabel = new QLabel("09:00", this);
    m_timeValueLabel->setStyleSheet("font-weight: bold; color: #1e40af; min-width: 55px;");

    ctrlLayout->addWidget(m_lineTitleLabel);
    ctrlLayout->addWidget(m_lineCombo);
    ctrlLayout->addSpacing(10);
    ctrlLayout->addWidget(m_dateTitleLabel);
    ctrlLayout->addWidget(m_dateCombo);
    ctrlLayout->addSpacing(10);
    ctrlLayout->addWidget(m_modeTitleLabel);
    ctrlLayout->addWidget(m_colorModeCombo);
    ctrlLayout->addSpacing(10);
    ctrlLayout->addWidget(m_sliderTitleLabel);
    ctrlLayout->addWidget(m_timeSlider);
    ctrlLayout->addWidget(m_timeValueLabel);
    ctrlLayout->addStretch();
    mainLayout->addLayout(ctrlLayout);

    // Bottleneck Banner
    m_bottleneckBanner = new QLabel(this);
    m_bottleneckBanner->setWordWrap(true);
    m_bottleneckBanner->setStyleSheet(
        "background-color: #fef3c7; border: 1px solid #f59e0b; border-radius: 6px; padding: 8px; color: #92400e; font-weight: bold;"
    );
    mainLayout->addWidget(m_bottleneckBanner);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    // QGraphicsView für Materialfluss
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setMinimumHeight(230);
    m_view->setStyleSheet("background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 6px;");
    m_scene->installEventFilter(this);
    splitter->addWidget(m_view);

    // Engpassanalyse-Tabelle unten
    m_bottleneckTable = new QTableWidget(this);
    m_bottleneckTable->setColumnCount(7);
    m_bottleneckTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_bottleneckTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bottleneckTable->setAlternatingRowColors(true);
    m_bottleneckTable->horizontalHeader()->setStretchLastSection(true);
    splitter->addWidget(m_bottleneckTable);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter);

    connect(m_lineCombo, &QComboBox::currentIndexChanged, this, &LineFlowWidget::onSelectionChanged);
    connect(m_dateCombo, &QComboBox::currentIndexChanged, this, &LineFlowWidget::onSelectionChanged);
    connect(m_colorModeCombo, &QComboBox::currentIndexChanged, this, &LineFlowWidget::onSelectionChanged);
    connect(m_timeSlider, &QSlider::valueChanged, this, &LineFlowWidget::onTimeSliderChanged);
    connect(m_bottleneckTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* item = m_bottleneckTable->item(row, 0)) {
            emit anlageDoubleClicked(item->text());
        }
    });
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, &LineFlowWidget::retranslateUi);

    retranslateUi();
}

void LineFlowWidget::refreshView() {
    if (!m_repo) return;

    m_lineCombo->blockSignals(true);
    m_lineCombo->clear();
    for (const auto& l : m_repo->linien()) {
        m_lineCombo->addItem(QString("%1 – %2 (%3)").arg(l.linieId, l.bezeichnung, l.halle), l.linieId);
    }
    m_lineCombo->blockSignals(false);

    m_dateCombo->blockSignals(true);
    m_dateCombo->clear();
    const QList<QDate> dates = m_repo->allAvailableDates();
    for (const QDate& d : dates) {
        m_dateCombo->addItem(d.toString("yyyy-MM-dd"), d);
    }
    if (m_dateCombo->count() > 0) m_dateCombo->setCurrentIndex(0);
    m_dateCombo->blockSignals(false);

    rebuildScene();
}

void LineFlowWidget::onSelectionChanged() {
    const int mode = m_colorModeCombo->currentIndex();
    m_timeSlider->setEnabled(mode == 1);
    rebuildScene();
}

void LineFlowWidget::onTimeSliderChanged(int minutesFrom6Am) {
    const QDate selDate = m_dateCombo->currentData().toDate();
    QDateTime baseTime(selDate.isValid() ? selDate : QDate(2026, 9, 14), QTime(6, 0, 0));
    QDateTime curTime = baseTime.addSecs(minutesFrom6Am * 60);
    m_timeValueLabel->setText(curTime.toString("HH:mm"));
    if (m_colorModeCombo->currentIndex() == 1) {
        rebuildScene();
    }
}

void LineFlowWidget::rebuildScene() {
    m_scene->clear();
    if (!m_repo || m_lineCombo->count() == 0) return;

    const QString linieId = m_lineCombo->currentData().toString();
    const QDate selDate = m_dateCombo->currentData().toDate();
    const int colorMode = m_colorModeCombo->currentIndex(); // 0 = OEE, 1 = Uhrzeit-Zustand, 2 = Engpass

    // Anlagen der Linie nach Position sortieren
    QList<Anlage> stations;
    for (const auto& a : m_repo->anlagen()) {
        if (a.linieId == linieId) stations.append(a);
    }
    std::sort(stations.begin(), stations.end(), [](const Anlage& x, const Anlage& y) {
        return x.position < y.position;
    });

    // Engpassanalyse für diese Linie und den gewählten Tag berechnen
    const QList<BottleneckEntry> bottlenecks = OeeCalculator::analyzeLineBottlenecks(linieId, *m_repo, selDate);
    QMap<QString, BottleneckEntry> bnMap;
    QString primaryBnId;
    QString primaryBnName;
    for (const auto& b : bottlenecks) {
        bnMap.insert(b.anlageId, b);
        if (b.isPrimaryBottleneck) {
            primaryBnId = b.anlageId;
            primaryBnName = b.bezeichnung;
        }
    }

    // Engpass-Banner aktualisieren
    if (!primaryBnId.isEmpty()) {
        const auto& pb = bnMap[primaryBnId];
        m_bottleneckBanner->setText(trBi(
            "🔥 Identifizierter Haupt-Engpass der Linie %1 am %2: %3 (%4) – Soll-Takt: %5 s | Eigene Störzeit: %6 min | Verursachter Materialmangel/Rückstau bei Nachbarn: %7 min (Doppelklick auf Kasten für Details)",
            "🔥 Identified Primary Bottleneck of Line %1 on %2: %3 (%4) – Cycle Time: %5 s | Fault Time: %6 min | Caused Neighbor Starvation/Blockage: %7 min (Double-click box for details)"
        ).arg(linieId, selDate.toString("dd.MM.yyyy"), primaryBnId, primaryBnName)
         .arg(pb.sollTaktzeitS, 0, 'f', 1)
         .arg(pb.stoerzeitMin, 0, 'f', 1)
         .arg(pb.causedStarvationDownstreamMin + pb.causedBlockageUpstreamMin, 0, 'f', 1));
    }

    // Tabelle füllen
    m_bottleneckTable->setRowCount(bottlenecks.size());
    for (int r = 0; r < bottlenecks.size(); ++r) {
        const auto& b = bottlenecks[r];
        auto makeItem = [&](const QString& txt) {
            auto* it = new QTableWidgetItem(txt);
            if (b.isPrimaryBottleneck) {
                it->setBackground(QColor("#fef3c7"));
                QFont f = it->font();
                f.setBold(true);
                it->setFont(f);
            }
            return it;
        };
        m_bottleneckTable->setItem(r, 0, makeItem(b.anlageId));
        m_bottleneckTable->setItem(r, 1, makeItem(QString::number(b.position)));
        m_bottleneckTable->setItem(r, 2, makeItem(b.bezeichnung));
        m_bottleneckTable->setItem(r, 3, makeItem(QString("%1 s").arg(b.sollTaktzeitS, 0, 'f', 1)));
        m_bottleneckTable->setItem(r, 4, makeItem(QString("%1 min").arg(b.stoerzeitMin, 0, 'f', 1)));
        m_bottleneckTable->setItem(r, 5, makeItem(QString("%1 min / %2 min")
            .arg(b.causedStarvationDownstreamMin, 0, 'f', 1)
            .arg(b.causedBlockageUpstreamMin, 0, 'f', 1)));
        m_bottleneckTable->setItem(r, 6, makeItem(b.isPrimaryBottleneck
            ? trBi("★ HAUPT-ENGPASS (Score %1)", "★ PRIMARY BOTTLENECK (Score %1)").arg(b.bottleneckScore, 0, 'f', 1)
            : QString::number(b.bottleneckScore, 'f', 1)));
    }

    // Grafische Kästen in Materialflussrichtung zeichnen (2 Reihen falls > 7 Stationen)
    const int boxW = 155;
    const int boxH = 92;
    const int gapX = 36;
    const int gapY = 45;
    const int colsPerRow = 7;

    const QDateTime queryTime = QDateTime(selDate, QTime(6, 0, 0)).addSecs(m_timeSlider->value() * 60);

    for (int i = 0; i < stations.size(); ++i) {
        const Anlage& a = stations[i];
        const int row = i / colsPerRow;
        const int col = i % colsPerRow;
        const double x = 20 + col * (boxW + gapX);
        const double y = 20 + row * (boxH + gapY);

        auto proto = m_repo->getProtokollLazy(a.anlageId, selDate);
        OeeMetrics dm = proto ? OeeCalculator::calculateDailyOee(*proto, a.sollTaktzeitS) : OeeMetrics{};

        QColor boxColor("#e2e8f0");
        QString subLine;

        if (colorMode == 0) {
            // Einfärbung nach Tages-OEE
            if (!dm.isValid) {
                boxColor = QColor("#cbd5e1");
                subLine = trBi("Keine Schicht / N/A", "No shift / N/A");
            } else {
                if (dm.oee >= 0.80) boxColor = QColor("#bbf7d0");
                else if (dm.oee >= 0.70) boxColor = QColor("#fef08a");
                else boxColor = QColor("#fecaca");
                subLine = QString("OEE: %1% | V:%2%").arg(dm.oee * 100.0, 0, 'f', 1).arg(dm.verfuegbarkeit * 100.0, 0, 'f', 0);
            }
        } else if (colorMode == 1) {
            // Einfärbung nach Zustand zur gewählten Uhrzeit (Zeitschieberegler)
            ZustandTyp activeState = ZustandTyp::Unbekannt;
            bool foundState = false;
            if (proto && proto->loadSuccess) {
                for (const auto& s : proto->schichten) {
                    if (!s.geplant) continue;
                    for (const auto& z : s.zustaende) {
                        if (z.beginn <= queryTime && queryTime < z.ende) {
                            activeState = z.typ;
                            foundState = true;
                            break;
                        }
                    }
                }
            }
            if (foundState) {
                boxColor = colorForZustandTyp(activeState).lighter(145);
                subLine = displayNameForZustandTyp(activeState);
            } else {
                boxColor = QColor("#e2e8f0");
                subLine = trBi("Schichtfrei", "Off-shift");
            }
        } else {
            // Einfärbung nach Engpass
            const auto& bn = bnMap.value(a.anlageId);
            if (bn.isPrimaryBottleneck) {
                boxColor = QColor("#fca5a5");
                subLine = trBi("★ ENGPASS (%1s)", "★ BOTTLENECK (%1s)").arg(a.sollTaktzeitS, 0, 'f', 1);
            } else {
                boxColor = QColor("#dbeafe");
                subLine = QString("Score: %1").arg(bn.bottleneckScore, 0, 'f', 1);
            }
        }

        // Pfeil zur nächsten Station
        if (i + 1 < stations.size()) {
            const int nextRow = (i + 1) / colsPerRow;
            if (nextRow == row) {
                const double ax1 = x + boxW;
                const double ay = y + boxH / 2.0;
                const double ax2 = ax1 + gapX;
                m_scene->addLine(ax1, ay, ax2, ay, QPen(QColor("#475569"), 2.2));
                QPolygonF arrowHead;
                arrowHead << QPointF(ax2, ay) << QPointF(ax2 - 8, ay - 5) << QPointF(ax2 - 8, ay + 5);
                m_scene->addPolygon(arrowHead, QPen(QColor("#475569")), QBrush(QColor("#475569")));
            }
        }

        const bool isPrimary = bnMap.value(a.anlageId).isPrimaryBottleneck;
        auto* rectItem = m_scene->addRect(
            QRectF(x, y, boxW, boxH),
            QPen(isPrimary ? QColor("#dc2626") : QColor("#475569"), isPrimary ? 3.0 : 1.5),
            QBrush(boxColor)
        );
        rectItem->setData(0, a.anlageId);
        rectItem->setCursor(Qt::PointingHandCursor);
        rectItem->setToolTip(trBi("Doppelklick zum Öffnen von %1 (%2)",
                                  "Double-click to open %1 (%2)").arg(a.anlageId, a.bezeichnung));

        auto* txt = m_scene->addText(QString("Pos %1 | %2\n%3\n%4")
            .arg(a.position)
            .arg(a.anlageId)
            .arg(a.bezeichnung.left(19))
            .arg(subLine));
        txt->setPos(x + 6, y + 6);
        txt->setDefaultTextColor(QColor("#0f172a"));
        QFont f = font();
        f.setPointSize(8);
        f.setBold(true);
        txt->setFont(f);
        txt->setData(0, a.anlageId);
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-15, -15, 15, 15));
}

bool LineFlowWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_scene && event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        auto* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
        const auto items = m_scene->items(mouseEvent->scenePos());
        for (auto* it : items) {
            const QString aid = it->data(0).toString();
            if (!aid.isEmpty()) {
                emit anlageDoubleClicked(aid);
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void LineFlowWidget::retranslateUi() {
    m_lineTitleLabel->setText(trBi("Fertigungslinie:", "Production Line:"));
    m_dateTitleLabel->setText(trBi("Tag:", "Date:"));
    m_modeTitleLabel->setText(trBi("Einfärbung:", "Color Mode:"));
    m_sliderTitleLabel->setText(trBi("Zeitschieberegler (Uhrzeit):", "Time Slider:"));

    const int curMode = std::max(0, m_colorModeCombo->currentIndex());
    m_colorModeCombo->blockSignals(true);
    m_colorModeCombo->clear();
    m_colorModeCombo->addItem(trBi("Nach Tages-OEE (Grün/Gelb/Rot)", "By Daily OEE (Green/Yellow/Red)"));
    m_colorModeCombo->addItem(trBi("Nach Live-Zustand zur gewählten Uhrzeit", "By Live State at Selected Time"));
    m_colorModeCombo->addItem(trBi("Nach Engpass-Score", "By Bottleneck Score"));
    m_colorModeCombo->setCurrentIndex(curMode);
    m_colorModeCombo->blockSignals(false);

    m_bottleneckTable->setHorizontalHeaderLabels({
        trBi("Anlage-ID", "Machine ID"),
        trBi("Pos.", "Pos."),
        trBi("Bezeichnung", "Description"),
        trBi("Soll-Taktzeit", "Cycle Time"),
        trBi("Eigene Störzeit", "Own Fault Time"),
        trBi("Verursachter Mangel / Rückstau", "Caused Starvation / Blockage"),
        trBi("Engpass-Bewertung", "Bottleneck Rating")
    });

    rebuildScene();
}

} // namespace RoboCell
