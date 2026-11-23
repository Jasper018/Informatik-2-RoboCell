#pragma once

#include "model/Entities.h"
#include "logic/OeeCalculator.h"
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QTableWidget>

namespace RoboCell {

class DataRepository;

/**
 * @brief Grafische Linienansicht (QGraphicsView) mit Zeitschieberegler & Engpassanalyse (Erweiterungen 7 & 8).
 */
class LineFlowWidget : public QWidget {
    Q_OBJECT
public:
    explicit LineFlowWidget(DataRepository* repo, QWidget* parent = nullptr);

    void refreshView();

signals:
    void anlageDoubleClicked(const QString& anlageId);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onSelectionChanged();
    void onTimeSliderChanged(int minutesFrom6Am);
    void retranslateUi();

private:
    void rebuildScene();

    DataRepository* m_repo = nullptr;
    QComboBox* m_lineCombo = nullptr;
    QComboBox* m_dateCombo = nullptr;
    QComboBox* m_colorModeCombo = nullptr;
    QSlider* m_timeSlider = nullptr;
    QLabel* m_timeValueLabel = nullptr;
    QLabel* m_lineTitleLabel = nullptr;
    QLabel* m_modeTitleLabel = nullptr;
    QLabel* m_dateTitleLabel = nullptr;
    QLabel* m_sliderTitleLabel = nullptr;
    QLabel* m_bottleneckBanner = nullptr;

    QGraphicsScene* m_scene = nullptr;
    QGraphicsView* m_view = nullptr;
    QTableWidget* m_bottleneckTable = nullptr;
};

} // namespace RoboCell
