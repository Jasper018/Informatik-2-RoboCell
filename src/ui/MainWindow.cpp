#include "ui/MainWindow.h"
#include "ui/AnlageDetailDialog.h"
#include "ui/EditDialogs.h"
#include "ui/LoginDialog.h"
#include "io/DataRepository.h"
#include "core/AuthManager.h"
#include "core/LanguageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QStatusBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QtConcurrent>

namespace RoboCell {

MainWindow::MainWindow(DataRepository* repo, QWidget* parent)
    : QMainWindow(parent), m_repo(repo)
{
    resize(1340, 840);
    setMinimumSize(1100, 700);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setSpacing(8);
    rootLayout->setContentsMargins(12, 10, 12, 8);

    // ========================================================================
    // 1. Top Action & Role Bar
    // ========================================================================
    auto* topBar = new QHBoxLayout();
    m_appTitleLabel = new QLabel(this);
    m_appTitleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0f172a;");

    m_userBadgeLabel = new QLabel(this);
    m_userBadgeLabel->setStyleSheet(
        "background-color: #dbeafe; border: 1px solid #93c5fd; border-radius: 5px; padding: 4px 10px; color: #1e3a8a; font-weight: bold;"
    );

    m_switchUserBtn = new QPushButton(this);
    m_langBtn = new QPushButton(this);
    m_openDirBtn = new QPushButton(this);
    m_exportCsvBtn = new QPushButton(this);
    m_exportCsvBtn->setStyleSheet("background-color: #16a34a; color: white; font-weight: bold; padding: 5px 10px; border-radius: 5px;");
    m_saveSqliteBtn = new QPushButton(this);
    m_loadSqliteBtn = new QPushButton(this);

    topBar->addWidget(m_appTitleLabel);
    topBar->addStretch();
    topBar->addWidget(m_userBadgeLabel);
    topBar->addWidget(m_switchUserBtn);
    topBar->addWidget(m_langBtn);
    topBar->addWidget(m_openDirBtn);
    topBar->addWidget(m_loadSqliteBtn);
    topBar->addWidget(m_saveSqliteBtn);
    topBar->addWidget(m_exportCsvBtn);
    rootLayout->addLayout(topBar);

    // ========================================================================
    // 2. Main Tab Widget
    // ========================================================================
    m_mainTabs = new QTabWidget(this);
    rootLayout->addWidget(m_mainTabs, 1);

    // ------------------------------------------------------------------------
    // TAB 1: Anlagenliste & Mehrstufige Filter
    // ------------------------------------------------------------------------
    auto* tabAnlagen = new QWidget(this);
    auto* anlagenLayout = new QVBoxLayout(tabAnlagen);

    auto* filterGroup = new QGroupBox(this);
    auto* filterGrid = new QGridLayout(filterGroup);
    filterGrid->setSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setClearButtonEnabled(true);

    m_linieFilterCombo = new QComboBox(this);
    m_typFilterCombo = new QComboBox(this);
    m_statusFilterCombo = new QComboBox(this);

    m_maxBaujahrSpin = new QSpinBox(this);
    m_maxBaujahrSpin->setRange(2000, 2035);
    m_maxBaujahrSpin->setSpecialValueText(trBi("Alle Baujahre", "All Years"));
    m_maxBaujahrSpin->setValue(2000); // 2000 = special value "Alle"

    m_onlyConspicuousCheck = new QCheckBox(this);
    m_onlyWarningsCheck = new QCheckBox(this);
    m_resetFilterBtn = new QPushButton(this);

    filterGrid->addWidget(new QLabel(trBi("Suche (ID, Bez., Linie):", "Search (ID, Desc., Line):"), this), 0, 0);
    filterGrid->addWidget(m_searchEdit, 0, 1);
    filterGrid->addWidget(new QLabel(trBi("Linie:", "Line:"), this), 0, 2);
    filterGrid->addWidget(m_linieFilterCombo, 0, 3);
    filterGrid->addWidget(new QLabel(trBi("Typ:", "Type:"), this), 0, 4);
    filterGrid->addWidget(m_typFilterCombo, 0, 5);
    filterGrid->addWidget(new QLabel(trBi("Status:", "Status:"), this), 0, 6);
    filterGrid->addWidget(m_statusFilterCombo, 0, 7);

    filterGrid->addWidget(new QLabel(trBi("Baujahr bis max.:", "Build Year max.:"), this), 1, 0);
    filterGrid->addWidget(m_maxBaujahrSpin, 1, 1);
    filterGrid->addWidget(m_onlyConspicuousCheck, 1, 2, 1, 2);
    filterGrid->addWidget(m_onlyWarningsCheck, 1, 4, 1, 2);
    filterGrid->addWidget(m_resetFilterBtn, 1, 6, 1, 2);
    anlagenLayout->addWidget(filterGroup);

    // TableView + ProxyModel
    m_tableModel = new AnlagenTableModel(m_repo, this);
    m_proxyModel = new AnlagenFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_tableModel);

    m_anlagenView = new QTableView(this);
    m_anlagenView->setModel(m_proxyModel);
    m_anlagenView->setSortingEnabled(true);
    m_anlagenView->sortByColumn(AnlagenTableModel::ColAnlageId, Qt::AscendingOrder);
    m_anlagenView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_anlagenView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_anlagenView->setAlternatingRowColors(true);
    m_anlagenView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_anlagenView->horizontalHeader()->setStretchLastSection(true);
    m_anlagenView->verticalHeader()->setVisible(false);
    anlagenLayout->addWidget(m_anlagenView, 1);

    // Action Bar below Anlagen table
    auto* actionRow = new QHBoxLayout();
    m_filterCountLabel = new QLabel(this);
    m_filterCountLabel->setStyleSheet("font-weight: bold; color: #334155;");
    m_detailBtn = new QPushButton(this);
    m_detailBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold; padding: 6px 14px; border-radius: 5px;");
    m_editAnlageBtn = new QPushButton(this);
    m_addMaintBtn = new QPushButton(this);

    actionRow->addWidget(m_filterCountLabel);
    actionRow->addStretch();
    actionRow->addWidget(m_detailBtn);
    actionRow->addWidget(m_editAnlageBtn);
    actionRow->addWidget(m_addMaintBtn);
    anlagenLayout->addLayout(actionRow);

    m_mainTabs->addTab(tabAnlagen, "");

    // ------------------------------------------------------------------------
    // TAB 2: Grafische Linienansicht & Engpassanalyse (Erweiterung 7 & 8)
    // ------------------------------------------------------------------------
    m_lineFlowWidget = new LineFlowWidget(m_repo, this);
    m_mainTabs->addTab(m_lineFlowWidget, "");

    // ------------------------------------------------------------------------
    // TAB 3: Störcodes & Pareto-Auswertung (Mindestanforderung 2 & Erweiterung 1)
    // ------------------------------------------------------------------------
    auto* tabStoer = new QWidget(this);
    auto* stoerLayout = new QVBoxLayout(tabStoer);
    auto* paretoTop = new QHBoxLayout();
    paretoTop->addWidget(new QLabel(trBi("<b>Pareto-Auswertung der Störcodes nach Linie filtern:</b>",
                                         "<b>Filter Fault Code Pareto Analysis by Line:</b>"), this));
    m_paretoLineCombo = new QComboBox(this);
    m_calcParetoBtn = new QPushButton(trBi("🔄 Pareto-Statistik über alle Schichtprotokolle berechnen",
                                           "🔄 Calculate Pareto Statistics Across All Shift Logs"), this);
    m_calcParetoBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold; padding: 5px 12px; border-radius: 5px;");
    paretoTop->addWidget(m_paretoLineCombo);
    paretoTop->addWidget(m_calcParetoBtn);
    paretoTop->addStretch();
    stoerLayout->addLayout(paretoTop);

    auto* stoerSplitter = new QSplitter(Qt::Horizontal, this);
    m_stoercodesTable = new QTableWidget(this);
    m_stoercodesTable->setColumnCount(3);
    m_stoercodesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_stoercodesTable->setAlternatingRowColors(true);
    m_stoercodesTable->horizontalHeader()->setStretchLastSection(true);

    m_paretoTable = new QTableWidget(this);
    m_paretoTable->setColumnCount(6);
    m_paretoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_paretoTable->setAlternatingRowColors(true);
    m_paretoTable->horizontalHeader()->setStretchLastSection(true);

    stoerSplitter->addWidget(m_stoercodesTable);
    stoerSplitter->addWidget(m_paretoTable);
    stoerSplitter->setStretchFactor(0, 2);
    stoerSplitter->setStretchFactor(1, 3);
    stoerLayout->addWidget(stoerSplitter, 1);
    m_mainTabs->addTab(tabStoer, "");

    // ------------------------------------------------------------------------
    // TAB 4: Linien, Produkte & Wartungen (Mindestanforderung 2 & Erweiterung 1)
    // ------------------------------------------------------------------------
    auto* tabMaster = new QWidget(this);
    auto* masterLayout = new QVBoxLayout(tabMaster);
    auto* masterSplitter = new QSplitter(Qt::Vertical, this);

    auto* topMasterWidget = new QWidget(this);
    auto* topMasterLayout = new QHBoxLayout(topMasterWidget);
    topMasterLayout->setContentsMargins(0, 0, 0, 0);

    m_linienTable = new QTableWidget(this);
    m_linienTable->setColumnCount(6);
    m_linienTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_linienTable->setAlternatingRowColors(true);
    m_linienTable->horizontalHeader()->setStretchLastSection(true);

    m_produkteTable = new QTableWidget(this);
    m_produkteTable->setColumnCount(4);
    m_produkteTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_produkteTable->setAlternatingRowColors(true);
    m_produkteTable->horizontalHeader()->setStretchLastSection(true);

    topMasterLayout->addWidget(m_linienTable, 3);
    topMasterLayout->addWidget(m_produkteTable, 2);
    masterSplitter->addWidget(topMasterWidget);

    auto* bottomMaintWidget = new QWidget(this);
    auto* bottomMaintLayout = new QVBoxLayout(bottomMaintWidget);
    bottomMaintLayout->setContentsMargins(0, 0, 0, 0);
    auto* maintHeader = new QHBoxLayout();
    maintHeader->addWidget(new QLabel(trBi("<b>Wartungshistorie (wartungen.csv):</b>", "<b>Maintenance History (wartungen.csv):</b>"), this));
    maintHeader->addStretch();
    m_newWartungTabBtn = new QPushButton(trBi("➕ Neue Wartung erfassen", "➕ Record New Maintenance"), this);
    maintHeader->addWidget(m_newWartungTabBtn);
    bottomMaintLayout->addLayout(maintHeader);

    m_wartungenTable = new QTableWidget(this);
    m_wartungenTable->setColumnCount(7);
    m_wartungenTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_wartungenTable->setAlternatingRowColors(true);
    m_wartungenTable->horizontalHeader()->setStretchLastSection(true);
    bottomMaintLayout->addWidget(m_wartungenTable, 1);

    masterSplitter->addWidget(bottomMaintWidget);
    masterLayout->addWidget(masterSplitter, 1);
    m_mainTabs->addTab(tabMaster, "");

    // ------------------------------------------------------------------------
    // TAB 5: Datenqualität & Fehlerprotokoll (Mindestanforderung 1 & 7)
    // ------------------------------------------------------------------------
    auto* tabIssues = new QWidget(this);
    auto* issuesLayout = new QVBoxLayout(tabIssues);
    m_issuesSummaryLabel = new QLabel(this);
    m_issuesSummaryLabel->setWordWrap(true);
    m_issuesSummaryLabel->setStyleSheet("background-color: #fff7ed; border: 1px solid #fb923c; border-radius: 6px; padding: 8px; color: #9a3412; font-weight: bold;");
    issuesLayout->addWidget(m_issuesSummaryLabel);

    m_issuesTable = new QTableWidget(this);
    m_issuesTable->setColumnCount(5);
    m_issuesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_issuesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_issuesTable->setAlternatingRowColors(true);
    m_issuesTable->horizontalHeader()->setStretchLastSection(true);
    issuesLayout->addWidget(m_issuesTable, 1);
    m_mainTabs->addTab(tabIssues, "");

    // ========================================================================
    // 3. StatusBar with Background Loader & Cache Info
    // ========================================================================
    m_bgProgressBar = new QProgressBar(this);
    m_bgProgressBar->setMaximumWidth(200);
    m_bgProgressBar->setRange(0, 0);
    m_bgProgressBar->hide();

    m_cacheStatusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_bgProgressBar);
    statusBar()->addPermanentWidget(m_cacheStatusLabel);

    m_bgWatcher = new QFutureWatcher<QMap<QString, AnomalyReport>>(this);

    // ========================================================================
    // Connections
    // ========================================================================
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_linieFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_typFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_statusFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_maxBaujahrSpin, &QSpinBox::valueChanged, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_onlyConspicuousCheck, &QCheckBox::toggled, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_onlyWarningsCheck, &QCheckBox::toggled, this, &MainWindow::onSearchOrFilterChanged);
    connect(m_resetFilterBtn, &QPushButton::clicked, this, &MainWindow::onResetFiltersClicked);

    connect(m_anlagenView, &QTableView::doubleClicked, this, &MainWindow::onAnlageDoubleClicked);
    connect(m_anlagenView, &QTableView::customContextMenuRequested, this, &MainWindow::onAnlageContextMenu);
    connect(m_detailBtn, &QPushButton::clicked, this, [this]() {
        const QString id = selectedAnlageId();
        if (!id.isEmpty()) openAnlageDetails(id);
    });
    connect(m_editAnlageBtn, &QPushButton::clicked, this, &MainWindow::onEditSelectedAnlage);
    connect(m_addMaintBtn, &QPushButton::clicked, this, &MainWindow::onAddMaintenanceForSelected);
    connect(m_newWartungTabBtn, &QPushButton::clicked, this, &MainWindow::onAddMaintenanceForSelected);

    connect(m_lineFlowWidget, &LineFlowWidget::anlageDoubleClicked, this, &MainWindow::openAnlageDetails);
    connect(m_issuesTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* item = m_issuesTable->item(row, 3)) {
            const QString id = item->text();
            if (m_repo->findAnlage(id)) {
                openAnlageDetails(id);
            }
        }
    });

    connect(m_calcParetoBtn, &QPushButton::clicked, this, [this]() {
        const QString lid = m_paretoLineCombo->currentData().toString();
        const auto items = OeeCalculator::calculateStoercodePareto(*m_repo, lid);
        m_paretoTable->setRowCount(items.size());
        for (int i = 0; i < items.size(); ++i) {
            const auto& it = items[i];
            m_paretoTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            m_paretoTable->setItem(i, 1, new QTableWidgetItem(it.code));
            m_paretoTable->setItem(i, 2, new QTableWidgetItem(it.kategorie));
            m_paretoTable->setItem(i, 3, new QTableWidgetItem(it.beschreibung));
            m_paretoTable->setItem(i, 4, new QTableWidgetItem(QString::number(it.count)));
            m_paretoTable->setItem(i, 5, new QTableWidgetItem(
                QString("%1 min (kum. %2%)").arg(it.totalMinutes, 0, 'f', 1).arg(it.cumulativePercent, 0, 'f', 1)));
        }
        m_cacheStatusLabel->setText(QString("LRU-Cache: %1 Protokolle | Hits: %2 | Misses: %3")
            .arg(m_repo->cacheSize()).arg(m_repo->cacheHits()).arg(m_repo->cacheMisses()));
    });

    connect(m_exportCsvBtn, &QPushButton::clicked, this, &MainWindow::onExportCsvClicked);
    connect(m_saveSqliteBtn, &QPushButton::clicked, this, &MainWindow::onSaveSqliteClicked);
    connect(m_loadSqliteBtn, &QPushButton::clicked, this, &MainWindow::onLoadSqliteClicked);
    connect(m_openDirBtn, &QPushButton::clicked, this, &MainWindow::onChangeDataDirClicked);
    connect(m_switchUserBtn, &QPushButton::clicked, this, &MainWindow::onSwitchUserClicked);
    connect(m_langBtn, &QPushButton::clicked, this, &MainWindow::onToggleLanguageClicked);

    connect(m_bgWatcher, &QFutureWatcher<QMap<QString, AnomalyReport>>::finished,
            this, &MainWindow::onBackgroundAnalysisFinished);

    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, &MainWindow::retranslateUi);
    connect(&AuthManager::instance(), &AuthManager::userChanged, this, &MainWindow::updateRolePermissionsUi);
    connect(m_repo, &DataRepository::anlageUpdated, this, [this]() {
        m_tableModel->refreshModel();
        refreshSecondaryTabs();
    });
    connect(m_repo, &DataRepository::wartungAdded, this, [this]() {
        refreshSecondaryTabs();
    });

    retranslateUi();
    updateRolePermissionsUi();
    refreshSecondaryTabs();
    m_lineFlowWidget->refreshView();
    m_anlagenView->resizeColumnsToContents();

    // Starte nicht-blockierende 14-Tage-Auswertung im Hintergrund (Erweiterung 5 & 6)
    startBackground14DaysAnalysis();
}

MainWindow::~MainWindow() {
    if (m_bgWatcher && m_bgWatcher->isRunning()) {
        m_bgWatcher->waitForFinished();
    }
}

QString MainWindow::selectedAnlageId() const {
    const QModelIndex proxyIdx = m_anlagenView->currentIndex();
    if (!proxyIdx.isValid()) return QString();
    return proxyIdx.siblingAtColumn(0).data(AnlagenTableModel::AnlageIdRole).toString();
}

void MainWindow::openAnlageDetails(const QString& anlageId) {
    if (anlageId.isEmpty()) return;
    AnlageDetailDialog dlg(anlageId, m_repo, this);
    dlg.exec();
    m_cacheStatusLabel->setText(QString("LRU-Cache: %1 Protokolle | Hits: %2 | Misses: %3")
        .arg(m_repo->cacheSize()).arg(m_repo->cacheHits()).arg(m_repo->cacheMisses()));
}

void MainWindow::onAnlageDoubleClicked(const QModelIndex& proxyIndex) {
    if (!proxyIndex.isValid()) return;
    const QString id = proxyIndex.siblingAtColumn(0).data(AnlagenTableModel::AnlageIdRole).toString();
    openAnlageDetails(id);
}

void MainWindow::onAnlageContextMenu(const QPoint& pos) {
    const QModelIndex idx = m_anlagenView->indexAt(pos);
    if (!idx.isValid()) return;
    const QString id = idx.siblingAtColumn(0).data(AnlagenTableModel::AnlageIdRole).toString();
    const Anlage* a = m_repo->findAnlage(id);
    if (!a) return;

    QMenu menu(this);
    auto* actDetail = menu.addAction(trBi("🔍 Details, Schichtprotokolle & OEE öffnen (%1)", "🔍 Open Details, Shift Logs & OEE (%1)").arg(id));
    menu.addSeparator();
    auto* actEdit = menu.addAction(trBi("✎ Stammdaten bearbeiten...", "✎ Edit Master Data..."));
    actEdit->setEnabled(AuthManager::instance().canChangeStatus());

    auto* actMaint = menu.addAction(trBi("🔧 Neue Wartung erfassen...", "🔧 Record New Maintenance..."));
    actMaint->setEnabled(AuthManager::instance().canRecordMaintenance());

    QMenu* statusMenu = menu.addMenu(trBi("⚡ Schnell-Status ändern", "⚡ Quick Change Status"));
    statusMenu->setEnabled(AuthManager::instance().canChangeStatus());
    auto* stBetrieb = statusMenu->addAction("in_betrieb");
    auto* stStoer = statusMenu->addAction("stoerung");
    auto* stWart = statusMenu->addAction("wartung");
    auto* stUmbau = statusMenu->addAction("umbau");

    QAction* chosen = menu.exec(m_anlagenView->viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == actDetail) {
        openAnlageDetails(id);
    } else if (chosen == actEdit) {
        onEditSelectedAnlage();
    } else if (chosen == actMaint) {
        onAddMaintenanceForSelected();
    } else if (chosen == stBetrieb || chosen == stStoer || chosen == stWart || chosen == stUmbau) {
        Anlage copy = *a;
        copy.rawStatus = chosen->text();
        copy.status = parseAnlagenStatus(copy.rawStatus);
        m_repo->updateAnlage(copy);
    }
}

void MainWindow::onSearchOrFilterChanged() {
    m_proxyModel->setSearchText(m_searchEdit->text());
    m_proxyModel->setLinieFilter(m_linieFilterCombo->currentData().toString());
    m_proxyModel->setTypFilter(m_typFilterCombo->currentData().toInt());
    m_proxyModel->setStatusFilter(m_statusFilterCombo->currentData().toInt());
    const int maxYear = m_maxBaujahrSpin->value();
    m_proxyModel->setMaxBaujahrFilter(maxYear > 2000 ? maxYear : 0);
    m_proxyModel->setOnlyConspicuous(m_onlyConspicuousCheck->isChecked());
    m_proxyModel->setOnlyDataWarnings(m_onlyWarningsCheck->isChecked());

    m_filterCountLabel->setText(trBi("Angezeigte Anlagen: %1 von %2", "Displayed Machines: %1 of %2")
        .arg(m_proxyModel->rowCount()).arg(m_tableModel->rowCount()));
}

void MainWindow::onResetFiltersClicked() {
    m_searchEdit->clear();
    m_linieFilterCombo->setCurrentIndex(0);
    m_typFilterCombo->setCurrentIndex(0);
    m_statusFilterCombo->setCurrentIndex(0);
    m_maxBaujahrSpin->setValue(2000);
    m_onlyConspicuousCheck->setChecked(false);
    m_onlyWarningsCheck->setChecked(false);
    m_proxyModel->resetAllFilters();
    onSearchOrFilterChanged();
}

void MainWindow::onEditSelectedAnlage() {
    const QString id = selectedAnlageId();
    if (id.isEmpty()) return;
    const Anlage* a = m_repo->findAnlage(id);
    if (!a) return;

    AnlageEditDialog dlg(*a, m_repo, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_repo->updateAnlage(dlg.updatedAnlage());
    }
}

void MainWindow::onAddMaintenanceForSelected() {
    QString id = selectedAnlageId();
    if (id.isEmpty() && !m_repo->anlagen().isEmpty()) {
        id = m_repo->anlagen().first().anlageId;
    }
    WartungAddDialog dlg(id, m_repo, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_repo->addWartung(dlg.createdWartung());
        QMessageBox::information(this,
            trBi("Wartung erfasst", "Maintenance Recorded"),
            trBi("Wartungseintrag wurde hinzugefügt.", "Maintenance entry was added."));
    }
}

void MainWindow::onExportCsvClicked() {
    QString err;
    if (m_repo->exportModifiedCsv(m_repo->dataDirectory(), &err)) {
        QMessageBox::information(this,
            trBi("CSV-Export erfolgreich", "CSV Export Successful"),
            trBi("Die aktualisierten Stammdaten wurden erfolgreich exportiert nach:\n• %1/anlagen_out.csv\n• %1/wartungen_out.csv",
                 "Updated master data successfully exported to:\n• %1/anlagen_out.csv\n• %1/wartungen_out.csv")
                .arg(m_repo->dataDirectory()));
    } else {
        QMessageBox::critical(this, trBi("Export-Fehler", "Export Error"), err);
    }
}

void MainWindow::onSaveSqliteClicked() {
    const QString path = QFileDialog::getSaveFileName(
        this,
        trBi("Stammdaten in SQLite-Datenbank speichern", "Save Master Data to SQLite Database"),
        QDir(m_repo->dataDirectory()).filePath("RoboCell_daten.sqlite"),
        "SQLite (*.sqlite *.db)"
    );
    if (path.isEmpty()) return;

    QString err;
    if (m_repo->saveToSqlite(path, &err)) {
        QMessageBox::information(this, trBi("SQLite-Persistenz", "SQLite Persistence"),
                                 trBi("Stammdaten erfolgreich in SQLite gespeichert:\n%1", "Master data saved to SQLite:\n%1").arg(path));
    } else {
        QMessageBox::critical(this, trBi("SQLite-Fehler", "SQLite Error"), err);
    }
}

void MainWindow::onLoadSqliteClicked() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        trBi("Stammdaten aus SQLite-Datenbank laden", "Load Master Data from SQLite Database"),
        QDir(m_repo->dataDirectory()).filePath("RoboCell_daten.sqlite"),
        "SQLite (*.sqlite *.db)"
    );
    if (path.isEmpty()) return;

    if (m_repo->loadFromSqlite(path)) {
        m_tableModel->refreshModel();
        refreshSecondaryTabs();
        m_lineFlowWidget->refreshView();
        QMessageBox::information(this, trBi("SQLite geladen", "SQLite Loaded"),
                                 trBi("Anlagen-Stammdaten wurden aus SQLite geladen.", "Machine master data loaded from SQLite."));
    } else {
        QMessageBox::warning(this, trBi("SQLite-Fehler", "SQLite Error"),
                             trBi("Datenbank konnte nicht geladen werden.", "Could not load database."));
    }
}

void MainWindow::onChangeDataDirClicked() {
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        trBi("RoboCell_Daten Ordner auswählen", "Select RoboCell_Daten Directory"),
        m_repo->dataDirectory()
    );
    if (dir.isEmpty()) return;

    m_repo->loadAllFromDirectory(dir);
    m_tableModel->refreshModel();
    refreshSecondaryTabs();
    m_lineFlowWidget->refreshView();
    startBackground14DaysAnalysis();
}

void MainWindow::onSwitchUserClicked() {
    LoginDialog dlg(this);
    dlg.exec();
}

void MainWindow::onToggleLanguageClicked() {
    auto& lm = LanguageManager::instance();
    lm.setLanguage(lm.currentLanguage() == AppLanguage::German ? AppLanguage::English : AppLanguage::German);
}

void MainWindow::startBackground14DaysAnalysis() {
    if (m_bgWatcher->isRunning()) return;
    m_bgProgressBar->show();
    statusBar()->showMessage(trBi("Lade alle 14 Protokolltage im Hintergrund für Trend- & OEE-Auswertung...",
                                  "Loading all 14 log days in background for trend & OEE evaluation..."));

    DataRepository* repoPtr = m_repo;
    const QList<Anlage> anlagenCopy = m_repo->anlagen();

    QFuture<QMap<QString, AnomalyReport>> future = QtConcurrent::run([repoPtr, anlagenCopy]() {
        QMap<QString, AnomalyReport> results;
        for (const auto& a : anlagenCopy) {
            const auto protos = repoPtr->loadAllProtokolleForAnlage(a.anlageId);
            const auto [daily, rep] = OeeCalculator::analyze14DaysTrend(a, protos);
            results.insert(a.anlageId, rep);
        }
        return results;
    });

    m_bgWatcher->setFuture(future);
}

void MainWindow::onBackgroundAnalysisFinished() {
    m_bgProgressBar->hide();
    const QMap<QString, AnomalyReport> reports = m_bgWatcher->result();
    m_tableModel->updateCalculatedMetrics(reports);
    m_anlagenView->resizeColumnsToContents();
    refreshSecondaryTabs();

    m_cacheStatusLabel->setText(QString("LRU-Cache: %1 Protokolle | Hits: %2 | Misses: %3")
        .arg(m_repo->cacheSize()).arg(m_repo->cacheHits()).arg(m_repo->cacheMisses()));
    statusBar()->showMessage(trBi("✓ 14-Tage-Hintergrundanalyse für alle Anlagen abgeschlossen.",
                                  "✓ 14-day background analysis completed for all machines."), 6000);
}

void MainWindow::updateRolePermissionsUi() {
    auto& auth = AuthManager::instance();
    m_userBadgeLabel->setText(QString("👤 %1 (%2)").arg(auth.currentUsername(), auth.currentRoleDisplayName()));
    m_userBadgeLabel->setToolTip(auth.permissionsSummary());

    m_editAnlageBtn->setEnabled(auth.canChangeStatus());
    m_addMaintBtn->setEnabled(auth.canRecordMaintenance());
    m_newWartungTabBtn->setEnabled(auth.canRecordMaintenance());
    m_exportCsvBtn->setEnabled(auth.canExportData());
    m_saveSqliteBtn->setEnabled(auth.canEditMasterData());
}

void MainWindow::refreshSecondaryTabs() {
    // 1. Linien-Filter & Pareto-Linien-Combo aktualisieren
    m_linieFilterCombo->blockSignals(true);
    m_linieFilterCombo->clear();
    m_linieFilterCombo->addItem(trBi("Alle Linien", "All Lines"), "");

    m_paretoLineCombo->blockSignals(true);
    m_paretoLineCombo->clear();
    m_paretoLineCombo->addItem(trBi("Alle Linien (Gesamtwerk)", "All Lines (Entire Plant)"), "");

    for (const auto& l : m_repo->linien()) {
        m_linieFilterCombo->addItem(QString("%1 – %2").arg(l.linieId, l.bezeichnung), l.linieId);
        m_paretoLineCombo->addItem(QString("%1 – %2").arg(l.linieId, l.bezeichnung), l.linieId);
    }
    // Auch fehlerhafte Linien wie L9 in der Filter-Auswahl auffindbar machen
    m_linieFilterCombo->addItem(trBi("L9 (Fremdschlüsselfehler A040)", "L9 (Foreign Key Error A040)"), "L9");
    m_linieFilterCombo->blockSignals(false);
    m_paretoLineCombo->blockSignals(false);

    // 2. Störcodes-Tabelle füllen
    const auto& scList = m_repo->stoercodes();
    m_stoercodesTable->setRowCount(scList.size());
    for (int i = 0; i < scList.size(); ++i) {
        m_stoercodesTable->setItem(i, 0, new QTableWidgetItem(scList[i].code));
        m_stoercodesTable->setItem(i, 1, new QTableWidgetItem(scList[i].kategorie));
        m_stoercodesTable->setItem(i, 2, new QTableWidgetItem(scList[i].beschreibung));
    }

    // 3. Linien-Tabelle inkl. Durchschnitts-OEE je Linie füllen
    const auto& linList = m_repo->linien();
    m_linienTable->setRowCount(linList.size());
    for (int i = 0; i < linList.size(); ++i) {
        const auto& l = linList[i];
        int countAnlagen = 0;
        double sumOee = 0.0;
        int oeeCount = 0;
        for (const auto& a : m_repo->anlagen()) {
            if (a.linieId == l.linieId) {
                ++countAnlagen;
                if (const auto* rep = m_tableModel->anomalyForAnlage(a.anlageId)) {
                    if (rep->avgOee > 0.0) {
                        sumOee += rep->avgOee;
                        ++oeeCount;
                    }
                }
            }
        }
        m_linienTable->setItem(i, 0, new QTableWidgetItem(l.linieId));
        m_linienTable->setItem(i, 1, new QTableWidgetItem(l.bezeichnung));
        m_linienTable->setItem(i, 2, new QTableWidgetItem(l.halle));
        m_linienTable->setItem(i, 3, new QTableWidgetItem(l.rawSchichtmodell));
        m_linienTable->setItem(i, 4, new QTableWidgetItem(QString::number(countAnlagen)));
        m_linienTable->setItem(i, 5, new QTableWidgetItem(
            oeeCount > 0 ? QString("%1 %").arg((sumOee / oeeCount) * 100.0, 0, 'f', 1) : "-"));
    }

    // 4. Produkte-Tabelle füllen
    const auto& prodList = m_repo->produkte();
    m_produkteTable->setRowCount(prodList.size());
    for (int i = 0; i < prodList.size(); ++i) {
        const auto& p = prodList[i];
        m_produkteTable->setItem(i, 0, new QTableWidgetItem(p.produktId));
        m_produkteTable->setItem(i, 1, new QTableWidgetItem(p.linieId));
        m_produkteTable->setItem(i, 2, new QTableWidgetItem(p.bezeichnung));
        auto* stItem = new QTableWidgetItem(p.hasValidationWarning ? ("⚠ " + p.validationMessage) : "✓ OK");
        if (p.hasValidationWarning) stItem->setBackground(QColor("#ffedd5"));
        m_produkteTable->setItem(i, 3, stItem);
    }

    // 5. Wartungen-Tabelle füllen
    const auto& wartList = m_repo->wartungen();
    m_wartungenTable->setRowCount(wartList.size());
    for (int i = 0; i < wartList.size(); ++i) {
        const auto& w = wartList[i];
        m_wartungenTable->setItem(i, 0, new QTableWidgetItem(w.wartungId));
        m_wartungenTable->setItem(i, 1, new QTableWidgetItem(w.anlageId));
        m_wartungenTable->setItem(i, 2, new QTableWidgetItem(w.datum.toString("yyyy-MM-dd")));
        m_wartungenTable->setItem(i, 3, new QTableWidgetItem(w.art));
        m_wartungenTable->setItem(i, 4, new QTableWidgetItem(QString("%1 min").arg(w.dauerMin)));
        m_wartungenTable->setItem(i, 5, new QTableWidgetItem(w.geplant ? "ja" : "nein"));
        m_wartungenTable->setItem(i, 6, new QTableWidgetItem(
            w.hasValidationWarning ? ("⚠ " + w.validationMessage + " | " + w.bemerkung) : w.bemerkung));
    }

    // 6. Datenqualitäts- & Plausibilitätsbericht füllen (Mindestanforderung 1 & 7)
    const auto& issues = m_repo->validationIssues();
    m_issuesSummaryLabel->setText(trBi(
        "Beim Einlesen der globalen CSV-Dateien wurden %1 Plausibilitäts-/Datenfehler erkannt und sicher abgefangen. "
        "Doppelklicken Sie auf einen Eintrag, um die betroffene Anlage direkt zu öffnen und zu korrigieren.",
        "During global CSV import, %1 plausibility/data issues were detected and safely handled. "
        "Double-click an entry to open and fix the affected machine directly."
    ).arg(issues.size()));

    m_issuesTable->setRowCount(issues.size());
    const bool isDe = (LanguageManager::instance().currentLanguage() == AppLanguage::German);
    for (int i = 0; i < issues.size(); ++i) {
        const auto& iss = issues[i];
        QString sevText = "WARNUNG";
        QColor bg("#fef9c3");
        if (iss.severity == ValidationIssue::Severity::Error) {
            sevText = isDe ? "FEHLER" : "ERROR";
            bg = QColor("#fee2e2");
        } else {
            sevText = isDe ? "WARNUNG" : "WARNING";
        }
        auto* sevItem = new QTableWidgetItem(sevText);
        sevItem->setBackground(bg);
        m_issuesTable->setItem(i, 0, sevItem);
        m_issuesTable->setItem(i, 1, new QTableWidgetItem(iss.sourceFile));
        m_issuesTable->setItem(i, 2, new QTableWidgetItem(iss.lineNumber > 0 ? QString::number(iss.lineNumber) : "-"));
        m_issuesTable->setItem(i, 3, new QTableWidgetItem(iss.entityId));
        m_issuesTable->setItem(i, 4, new QTableWidgetItem(isDe ? iss.messageDe : iss.messageEn));
    }

    onSearchOrFilterChanged();
}

void MainWindow::retranslateUi() {
    setWindowTitle(trBi("RoboCell – OEE- & Anlagenmanagement (C++ / Qt 6)",
                        "RoboCell – OEE & Machine Management (C++ / Qt 6)"));
    m_appTitleLabel->setText(trBi("🤖 RoboCell – Fertigungskennzahlen & OEE",
                                  "🤖 RoboCell – Manufacturing KPIs & OEE"));

    m_switchUserBtn->setText(trBi("🔑 Rolle / Benutzer wechseln", "🔑 Switch Role / User"));
    m_langBtn->setText(LanguageManager::instance().currentLanguage() == AppLanguage::German ? "🌐 EN (English)" : "🌐 DE (Deutsch)");
    m_openDirBtn->setText(trBi("📂 Datenordner...", "📂 Data Folder..."));
    m_loadSqliteBtn->setText(trBi("🗄 SQLite laden", "🗄 Load SQLite"));
    m_saveSqliteBtn->setText(trBi("💾 SQLite speichern", "💾 Save SQLite"));
    m_exportCsvBtn->setText(trBi("📤 CSV Export (*_out.csv)", "📤 CSV Export (*_out.csv)"));

    m_mainTabs->setTabText(0, trBi("🏭 Anlagenliste & Filter (%1)", "🏭 Machine List & Filters (%1)").arg(m_repo->anlagen().size()));
    m_mainTabs->setTabText(1, trBi("🔀 Grafische Linienansicht & Engpassanalyse", "🔀 Graphical Line View & Bottleneck Analysis"));
    m_mainTabs->setTabText(2, trBi("⚡ Störcodes & Pareto-Statistik", "⚡ Fault Codes & Pareto Statistics"));
    m_mainTabs->setTabText(3, trBi("📋 Linien, Produkte & Wartungen", "📋 Lines, Products & Maintenance"));
    m_mainTabs->setTabText(4, trBi("🛡 Datenqualität & Fehlerprotokoll (%1)", "🛡 Data Quality & Issue Log (%1)").arg(m_repo->validationIssues().size()));

    m_searchEdit->setPlaceholderText(trBi("z. B. A025, Schweißroboter, L2, Kinetronic...",
                                          "e.g. A025, Welding Robot, L2, Kinetronic..."));

    // Typ-Filter Combo
    m_typFilterCombo->blockSignals(true);
    m_typFilterCombo->clear();
    m_typFilterCombo->addItem(trBi("Alle Typen", "All Types"), -1);
    m_typFilterCombo->addItem("cnc_beladung", static_cast<int>(AnlagenTyp::CncBeladung));
    m_typFilterCombo->addItem("schweissroboter", static_cast<int>(AnlagenTyp::Schweissroboter));
    m_typFilterCombo->addItem("montageroboter", static_cast<int>(AnlagenTyp::Montageroboter));
    m_typFilterCombo->addItem("schraubstation", static_cast<int>(AnlagenTyp::Schraubstation));
    m_typFilterCombo->addItem("klebestation", static_cast<int>(AnlagenTyp::Klebestation));
    m_typFilterCombo->addItem("pruefstation", static_cast<int>(AnlagenTyp::Pruefstation));
    m_typFilterCombo->addItem("laserbeschriftung", static_cast<int>(AnlagenTyp::Laserbeschriftung));
    m_typFilterCombo->addItem("palettierer", static_cast<int>(AnlagenTyp::Palettierer));
    m_typFilterCombo->blockSignals(false);

    // Status-Filter Combo
    m_statusFilterCombo->blockSignals(true);
    m_statusFilterCombo->clear();
    m_statusFilterCombo->addItem(trBi("Alle Status", "All Statuses"), -1);
    m_statusFilterCombo->addItem("in_betrieb", static_cast<int>(AnlagenStatus::InBetrieb));
    m_statusFilterCombo->addItem("stoerung", static_cast<int>(AnlagenStatus::Stoerung));
    m_statusFilterCombo->addItem("wartung", static_cast<int>(AnlagenStatus::Wartung));
    m_statusFilterCombo->addItem("umbau", static_cast<int>(AnlagenStatus::Umbau));
    m_statusFilterCombo->blockSignals(false);

    m_onlyConspicuousCheck->setText(trBi("🔍 Nur auffällige Anlagen (14T-Trend)", "🔍 Only Conspicuous Machines (14d Trend)"));
    m_onlyWarningsCheck->setText(trBi("⚠ Nur Anlagen mit Stammdaten-Warnung", "⚠ Only Machines with Data Warnings"));
    m_resetFilterBtn->setText(trBi("✖ Filter zurücksetzen", "✖ Reset Filters"));

    m_detailBtn->setText(trBi("📊 Details, Schichtprotokoll & OEE öffnen", "📊 Open Details, Shift Log & OEE"));
    m_editAnlageBtn->setText(trBi("✎ Stammdaten bearbeiten", "✎ Edit Master Data"));
    m_addMaintBtn->setText(trBi("🔧 Wartung erfassen", "🔧 Record Maintenance"));

    m_stoercodesTable->setHorizontalHeaderLabels({
        trBi("Code", "Code"), trBi("Kategorie", "Category"), trBi("Beschreibung", "Description")
    });
    m_paretoTable->setHorizontalHeaderLabels({
        trBi("Rang", "Rank"), trBi("Code", "Code"), trBi("Kategorie", "Category"),
        trBi("Beschreibung", "Description"), trBi("Häufigkeit", "Count"),
        trBi("Gesamtdauer (kum. %)", "Total Duration (cum. %)")
    });
    m_linienTable->setHorizontalHeaderLabels({
        trBi("Linie", "Line"), trBi("Bezeichnung", "Description"), trBi("Halle", "Hall"),
        trBi("Schichtmodell", "Shift Model"), trBi("Anlagen", "Machines"), trBi("Ø Linien-OEE", "Avg Line OEE")
    });
    m_produkteTable->setHorizontalHeaderLabels({
        trBi("Produkt-ID", "Product ID"), trBi("Linie", "Line"), trBi("Bezeichnung", "Description"), trBi("Prüfung", "Check")
    });
    m_wartungenTable->setHorizontalHeaderLabels({
        trBi("Wartung-ID", "Maint. ID"), trBi("Anlage", "Machine"), trBi("Datum", "Date"),
        trBi("Art", "Type"), trBi("Dauer", "Duration"), trBi("Geplant", "Planned"), trBi("Bemerkung", "Comment")
    });
    m_issuesTable->setHorizontalHeaderLabels({
        trBi("Typ", "Severity"), trBi("Datei", "File"), trBi("Zeile", "Line"),
        trBi("Objekt-ID", "Entity ID"), trBi("Plausibilitätsmeldung", "Plausibility Message")
    });

    updateRolePermissionsUi();
    refreshSecondaryTabs();
}

} // namespace RoboCell
