#pragma once

#include "model/Entities.h"
#include "logic/OeeCalculator.h"
#include "ui/AnlagenTableModel.h"
#include "ui/LineGraphicsView.h"
#include <QMainWindow>
#include <QTableView>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTabWidget>
#include <QFutureWatcher>

namespace RoboCell {

class DataRepository;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(DataRepository* repo, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSearchOrFilterChanged();
    void onResetFiltersClicked();
    void onAnlageDoubleClicked(const QModelIndex& proxyIndex);
    void onAnlageContextMenu(const QPoint& pos);

    void onEditSelectedAnlage();
    void onAddMaintenanceForSelected();
    void onExportCsvClicked();
    void onSaveSqliteClicked();
    void onLoadSqliteClicked();
    void onChangeDataDirClicked();
    void onSwitchUserClicked();
    void onToggleLanguageClicked();

    void startBackground14DaysAnalysis();
    void onBackgroundAnalysisFinished();

    void retranslateUi();
    void updateRolePermissionsUi();
    void refreshSecondaryTabs();

private:
    QString selectedAnlageId() const;
    void openAnlageDetails(const QString& anlageId);

    DataRepository* m_repo = nullptr;

    // Top Header / Status & Role Bar
    QLabel* m_appTitleLabel = nullptr;
    QLabel* m_userBadgeLabel = nullptr;
    QPushButton* m_switchUserBtn = nullptr;
    QPushButton* m_langBtn = nullptr;
    QPushButton* m_openDirBtn = nullptr;
    QPushButton* m_exportCsvBtn = nullptr;
    QPushButton* m_saveSqliteBtn = nullptr;
    QPushButton* m_loadSqliteBtn = nullptr;

    // Main TabWidget
    QTabWidget* m_mainTabs = nullptr;

    // Tab 1: Anlagenliste & Mehrstufige Filter
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_linieFilterCombo = nullptr;
    QComboBox* m_typFilterCombo = nullptr;
    QComboBox* m_statusFilterCombo = nullptr;
    QSpinBox* m_maxBaujahrSpin = nullptr;
    QCheckBox* m_onlyConspicuousCheck = nullptr;
    QCheckBox* m_onlyWarningsCheck = nullptr;
    QPushButton* m_resetFilterBtn = nullptr;
    QPushButton* m_detailBtn = nullptr;
    QPushButton* m_editAnlageBtn = nullptr;
    QPushButton* m_addMaintBtn = nullptr;
    QLabel* m_filterCountLabel = nullptr;

    AnlagenTableModel* m_tableModel = nullptr;
    AnlagenFilterProxyModel* m_proxyModel = nullptr;
    QTableView* m_anlagenView = nullptr;

    // Tab 2: Grafische Linienansicht & Engpassanalyse
    LineFlowWidget* m_lineFlowWidget = nullptr;

    // Tab 3: Störcodes & Pareto-Auswertung
    QComboBox* m_paretoLineCombo = nullptr;
    QPushButton* m_calcParetoBtn = nullptr;
    QTableWidget* m_stoercodesTable = nullptr;
    QTableWidget* m_paretoTable = nullptr;

    // Tab 4: Linien, Produkte & Wartungen
    QTableWidget* m_linienTable = nullptr;
    QTableWidget* m_produkteTable = nullptr;
    QTableWidget* m_wartungenTable = nullptr;
    QPushButton* m_newWartungTabBtn = nullptr;

    // Tab 5: Datenqualität & Fehlerprotokoll (Plausibilitätsprüfungen)
    QLabel* m_issuesSummaryLabel = nullptr;
    QTableWidget* m_issuesTable = nullptr;

    // Background 14-day analysis (Erweiterung 5 & 6)
    QProgressBar* m_bgProgressBar = nullptr;
    QLabel* m_cacheStatusLabel = nullptr;
    QFutureWatcher<QMap<QString, AnomalyReport>>* m_bgWatcher = nullptr;
};

} // namespace RoboCell
