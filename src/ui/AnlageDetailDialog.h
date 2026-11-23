#pragma once

#include "model/Entities.h"
#include "logic/OeeCalculator.h"
#include "ui/CustomCharts.h"
#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTabWidget>

namespace RoboCell {

class DataRepository;

/**
 * @brief Detaildialog einer Anlage mit Stammdaten, Linieninfos, Lazy-Loading-Tagesprotokoll,
 *        Zustands-Zeitleiste, kumulierten Gutteilen, OEE-Auswertung, Schichtvergleich,
 *        MTBF/MTTR, 14-Tage-Trend und Berichtsexport (Mindestanforderung 2, 5, 6, 7 & Erweiterungen).
 */
class AnlageDetailDialog : public QDialog {
    Q_OBJECT
public:
    AnlageDetailDialog(const QString& anlageId, DataRepository* repo, QWidget* parent = nullptr);

private slots:
    void onDateSelected(int index);
    void onEditMasterDataClicked();
    void onAddMaintenanceClicked();
    void onExportReportPdfClicked();
    void onExportReportCsvClicked();
    void refreshMasterDataHeader();

private:
    void loadSelectedDayLazy(const QDate& date);
    void populate14DaysTrendTab();
    void populateMaintenanceTab();

    QString m_anlageId;
    DataRepository* m_repo = nullptr;

    // Header / Stammdaten
    QLabel* m_titleLabel = nullptr;
    QLabel* m_masterInfoLabel = nullptr;
    QLabel* m_lineInfoLabel = nullptr;
    QLabel* m_warningBanner = nullptr;
    QPushButton* m_editBtn = nullptr;
    QPushButton* m_addMaintBtn = nullptr;
    QPushButton* m_exportPdfBtn = nullptr;
    QPushButton* m_exportCsvBtn = nullptr;

    // Tagesauswahl
    QComboBox* m_dateCombo = nullptr;
    QLabel* m_xmlPathLabel = nullptr;
    QLabel* m_degradationBanner = nullptr;

    // OEE KPI Cards
    QProgressBar* m_vBar = nullptr;
    QProgressBar* m_lBar = nullptr;
    QProgressBar* m_qBar = nullptr;
    QProgressBar* m_oeeBar = nullptr;
    QLabel* m_kpiDetailsLabel = nullptr;
    QLabel* m_mtbfMttrLabel = nullptr;

    // Visualisierungen (Tab 1: Tagesverlauf)
    StateTimelineWidget* m_timelineWidget = nullptr;
    CumulativePartsChartWidget* m_cumPartsWidget = nullptr;

    // Tabellen (Tab 2: Zeitanteile & Schichtvergleich)
    QTableWidget* m_stateDurationTable = nullptr;
    QTableWidget* m_shiftCompareTable = nullptr;

    // Tab 3: 14-Tage-Trend & Anomalieanalyse
    Trend14DaysChartWidget* m_trendChart = nullptr;
    QLabel* m_trendAnomalyLabel = nullptr;
    QTableWidget* m_trendTable = nullptr;

    // Tab 4: Wartungshistorie der Anlage
    QTableWidget* m_maintTable = nullptr;
};

} // namespace RoboCell
