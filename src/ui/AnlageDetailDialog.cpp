#include "ui/AnlageDetailDialog.h"
#include "ui/EditDialogs.h"
#include "io/DataRepository.h"
#include "core/AuthManager.h"
#include "core/LanguageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextDocument>
#include <QPrinter>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>

namespace RoboCell {

static QProgressBar* createKpiBar(const QString& colorHex, QWidget* parent) {
    auto* bar = new QProgressBar(parent);
    bar->setRange(0, 1000);
    bar->setTextVisible(true);
    bar->setMinimumHeight(24);
    bar->setStyleSheet(QString(
        "QProgressBar { border: 1px solid #cbd5e1; border-radius: 5px; background: #f1f5f9; text-align: center; font-weight: bold; color: #0f172a; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 4px; }"
    ).arg(colorHex));
    return bar;
}

AnlageDetailDialog::AnlageDetailDialog(const QString& anlageId, DataRepository* repo, QWidget* parent)
    : QDialog(parent), m_anlageId(anlageId), m_repo(repo)
{
    setMinimumSize(1080, 760);
    resize(1160, 820);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // 1. Header-Zeile mit Titel und Aktions-Buttons
    auto* topRow = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #0f172a;");

    m_editBtn = new QPushButton(trBi("✎ Stammdaten bearbeiten", "✎ Edit Master Data"), this);
    m_editBtn->setEnabled(AuthManager::instance().canChangeStatus());

    m_addMaintBtn = new QPushButton(trBi("🔧 Wartung erfassen", "🔧 Record Maintenance"), this);
    m_addMaintBtn->setEnabled(AuthManager::instance().canRecordMaintenance());

    m_exportCsvBtn = new QPushButton(trBi("📄 Bericht (CSV)", "📄 Report (CSV)"), this);
    m_exportPdfBtn = new QPushButton(trBi("🖨 Bericht (PDF)", "🖨 Report (PDF)"), this);

    topRow->addWidget(m_titleLabel);
    topRow->addStretch();
    topRow->addWidget(m_editBtn);
    topRow->addWidget(m_addMaintBtn);
    topRow->addWidget(m_exportCsvBtn);
    topRow->addWidget(m_exportPdfBtn);
    mainLayout->addLayout(topRow);

    // 2. Stammdaten- & Linien-Übersicht
    auto* infoGroup = new QGroupBox(trBi("Stammdaten & Linien-Zuordnung", "Master Data & Line Assignment"), this);
    auto* infoLayout = new QGridLayout(infoGroup);
    m_masterInfoLabel = new QLabel(this);
    m_masterInfoLabel->setWordWrap(true);
    m_lineInfoLabel = new QLabel(this);
    m_lineInfoLabel->setWordWrap(true);
    infoLayout->addWidget(m_masterInfoLabel, 0, 0);
    infoLayout->addWidget(m_lineInfoLabel, 0, 1);
    mainLayout->addWidget(infoGroup);

    // Warnbanner für Stammdaten-Anomalien
    m_warningBanner = new QLabel(this);
    m_warningBanner->setWordWrap(true);
    m_warningBanner->setStyleSheet("background-color: #ffedd5; border: 1px solid #f97316; border-radius: 5px; padding: 6px; color: #9a3412; font-weight: bold;");
    m_warningBanner->hide();
    mainLayout->addWidget(m_warningBanner);

    // 3. Protokolltag-Auswahl (Lazy Loading)
    auto* dayBar = new QHBoxLayout();
    auto* dayLbl = new QLabel(trBi("<b>Protokolltag auswählen (Lazy Loading):</b>", "<b>Select Log Day (Lazy Loading):</b>"), this);
    m_dateCombo = new QComboBox(this);
    m_dateCombo->setMinimumWidth(160);
    m_xmlPathLabel = new QLabel(this);
    m_xmlPathLabel->setStyleSheet("color: #475569; font-family: Consolas, monospace;");

    dayBar->addWidget(dayLbl);
    dayBar->addWidget(m_dateCombo);
    dayBar->addSpacing(12);
    dayBar->addWidget(m_xmlPathLabel);
    dayBar->addStretch();
    mainLayout->addLayout(dayBar);

    // Degradationsbanner bei fehlendem/defektem Protokoll oder nicht geplanten Schichten
    m_degradationBanner = new QLabel(this);
    m_degradationBanner->setWordWrap(true);
    m_degradationBanner->setStyleSheet("background-color: #fee2e2; border: 1px solid #ef4444; border-radius: 6px; padding: 8px; color: #991b1b; font-weight: bold;");
    m_degradationBanner->hide();
    mainLayout->addWidget(m_degradationBanner);

    // 4. OEE & Zuverlässigkeits-KPI-Panel
    auto* kpiGroup = new QGroupBox(trBi("Fachliche Tagesauswertung: OEE, Verfügbarkeit, Leistung, Qualität & MTBF/MTTR",
                                        "Daily Evaluation: OEE, Availability, Performance, Quality & MTBF/MTTR"), this);
    auto* kpiGrid = new QGridLayout(kpiGroup);

    m_vBar = createKpiBar("#2563eb", this);
    m_lBar = createKpiBar("#d97706", this);
    m_qBar = createKpiBar("#16a34a", this);
    m_oeeBar = createKpiBar("#0f172a", this);

    kpiGrid->addWidget(new QLabel(trBi("<b>Verfügbarkeit (V):</b>", "<b>Availability (V):</b>"), this), 0, 0);
    kpiGrid->addWidget(m_vBar, 0, 1);
    kpiGrid->addWidget(new QLabel(trBi("<b>Leistungsgrad (L):</b>", "<b>Performance (L):</b>"), this), 0, 2);
    kpiGrid->addWidget(m_lBar, 0, 3);
    kpiGrid->addWidget(new QLabel(trBi("<b>Qualitätsrate (Q):</b>", "<b>Quality Rate (Q):</b>"), this), 1, 0);
    kpiGrid->addWidget(m_qBar, 1, 1);
    kpiGrid->addWidget(new QLabel(trBi("<b>Gesamt-OEE (V·L·Q):</b>", "<b>Total OEE (V·L·Q):</b>"), this), 1, 2);
    kpiGrid->addWidget(m_oeeBar, 1, 3);

    m_kpiDetailsLabel = new QLabel(this);
    m_mtbfMttrLabel = new QLabel(this);
    m_mtbfMttrLabel->setStyleSheet("color: #1e40af; font-weight: bold;");
    kpiGrid->addWidget(m_kpiDetailsLabel, 2, 0, 1, 2);
    kpiGrid->addWidget(m_mtbfMttrLabel, 2, 2, 1, 2);
    mainLayout->addWidget(kpiGroup);

    // 5. Registerkarten für Grafiken, Zeitanteile/Schichtvergleich, 14-Tage-Trend & Wartungen
    auto* tabs = new QTabWidget(this);

    // Tab 1: Grafischer Tagesverlauf (Zeitleiste + Kumulierte Gutteile)
    auto* tabCharts = new QWidget(this);
    auto* chartsLayout = new QVBoxLayout(tabCharts);
    m_timelineWidget = new StateTimelineWidget(this);
    m_cumPartsWidget = new CumulativePartsChartWidget(this);
    chartsLayout->addWidget(m_timelineWidget, 1);
    chartsLayout->addWidget(m_cumPartsWidget, 1);
    tabs->addTab(tabCharts, trBi("📊 Zustands-Zeitleiste & Stückzahlverlauf", "📊 State Timeline & Cumulative Parts"));

    // Tab 2: Zeitanteile je Zustand & Schichtvergleich (Früh / Spät / Nacht)
    auto* tabTables = new QWidget(this);
    auto* tablesLayout = new QHBoxLayout(tabTables);

    m_stateDurationTable = new QTableWidget(this);
    m_stateDurationTable->setColumnCount(4);
    m_stateDurationTable->setHorizontalHeaderLabels({
        trBi("Zustandstyp", "State Type"),
        trBi("Dauer [Minuten]", "Duration [Minutes]"),
        trBi("Dauer [Sekunden]", "Duration [Seconds]"),
        trBi("Anteil an Schichtzeit", "Share of Shift Time")
    });
    m_stateDurationTable->horizontalHeader()->setStretchLastSection(true);
    m_stateDurationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_stateDurationTable->setAlternatingRowColors(true);

    m_shiftCompareTable = new QTableWidget(this);
    m_shiftCompareTable->setColumnCount(7);
    m_shiftCompareTable->setHorizontalHeaderLabels({
        trBi("Schicht", "Shift"),
        trBi("Geplant", "Planned"),
        trBi("V [%]", "V [%]"),
        trBi("L [%]", "L [%]"),
        trBi("Q [%]", "Q [%]"),
        trBi("OEE [%]", "OEE [%]"),
        trBi("Gut / Ausschuss", "Good / Scrap")
    });
    m_shiftCompareTable->horizontalHeader()->setStretchLastSection(true);
    m_shiftCompareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_shiftCompareTable->setAlternatingRowColors(true);

    tablesLayout->addWidget(m_stateDurationTable, 1);
    tablesLayout->addWidget(m_shiftCompareTable, 1);
    tabs->addTab(tabTables, trBi("⏱ Zeitanteile je Zustand & Schichtvergleich", "⏱ State Time Shares & Shift Comparison"));

    // Tab 3: 14-Tage-Trend & Fehleranalyse
    auto* tabTrend = new QWidget(this);
    auto* trendLayout = new QVBoxLayout(tabTrend);
    m_trendAnomalyLabel = new QLabel(this);
    m_trendAnomalyLabel->setWordWrap(true);
    m_trendAnomalyLabel->setStyleSheet("background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 5px; padding: 6px;");
    m_trendChart = new Trend14DaysChartWidget(this);
    m_trendTable = new QTableWidget(this);
    m_trendTable->setColumnCount(8);
    m_trendTable->setHorizontalHeaderLabels({
        trBi("Datum", "Date"),
        trBi("V [%]", "V [%]"),
        trBi("L [%]", "L [%]"),
        trBi("Q [%]", "Q [%]"),
        trBi("OEE [%]", "OEE [%]"),
        trBi("Gutteile", "Good Parts"),
        trBi("Ausschuss", "Scrap"),
        trBi("Störungen (Min.)", "Faults (Min.)")
    });
    m_trendTable->horizontalHeader()->setStretchLastSection(true);
    m_trendTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trendTable->setAlternatingRowColors(true);

    trendLayout->addWidget(m_trendAnomalyLabel);
    trendLayout->addWidget(m_trendChart, 3);
    trendLayout->addWidget(m_trendTable, 2);
    tabs->addTab(tabTrend, trBi("📈 14-Tage-Trend & Anomalieanalyse", "📈 14-Day Trend & Anomaly Analysis"));

    // Tab 4: Wartungshistorie der Anlage
    auto* tabMaint = new QWidget(this);
    auto* maintLayout = new QVBoxLayout(tabMaint);
    m_maintTable = new QTableWidget(this);
    m_maintTable->setColumnCount(6);
    m_maintTable->setHorizontalHeaderLabels({
        trBi("Wartungs-ID", "Maint. ID"),
        trBi("Datum", "Date"),
        trBi("Art", "Type"),
        trBi("Dauer [min]", "Duration [min]"),
        trBi("Geplant", "Planned"),
        trBi("Bemerkung", "Comment")
    });
    m_maintTable->horizontalHeader()->setStretchLastSection(true);
    m_maintTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_maintTable->setAlternatingRowColors(true);
    maintLayout->addWidget(m_maintTable);
    tabs->addTab(tabMaint, trBi("🔧 Wartungshistorie", "🔧 Maintenance History"));

    mainLayout->addWidget(tabs, 1);

    // Verbindungen
    connect(m_dateCombo, &QComboBox::currentIndexChanged, this, &AnlageDetailDialog::onDateSelected);
    connect(m_editBtn, &QPushButton::clicked, this, &AnlageDetailDialog::onEditMasterDataClicked);
    connect(m_addMaintBtn, &QPushButton::clicked, this, &AnlageDetailDialog::onAddMaintenanceClicked);
    connect(m_exportPdfBtn, &QPushButton::clicked, this, &AnlageDetailDialog::onExportReportPdfClicked);
    connect(m_exportCsvBtn, &QPushButton::clicked, this, &AnlageDetailDialog::onExportReportCsvClicked);

    refreshMasterDataHeader();

    // Verfügbare Protokolltage aus master_protokoll_index.csv füllen
    const QList<QDate> dates = m_repo->availableDatesForAnlage(m_anlageId);
    m_dateCombo->blockSignals(true);
    for (const QDate& d : dates) {
        m_dateCombo->addItem(d.toString("yyyy-MM-dd (ddd)"), d);
    }
    m_dateCombo->blockSignals(false);

    if (m_dateCombo->count() > 0) {
        m_dateCombo->setCurrentIndex(0);
        onDateSelected(0);
    } else {
        loadSelectedDayLazy(QDate());
    }

    populate14DaysTrendTab();
    populateMaintenanceTab();
}

void AnlageDetailDialog::refreshMasterDataHeader() {
    const Anlage* a = m_repo->findAnlage(m_anlageId);
    if (!a) return;

    setWindowTitle(trBi("Anlagendetails & Schichtprotokolle – %1 (%2)",
                        "Machine Details & Shift Logs – %1 (%2)").arg(a->anlageId, a->bezeichnung));
    m_titleLabel->setText(QString("%1 – %2").arg(a->anlageId, a->bezeichnung));

    m_masterInfoLabel->setText(trBi(
        "<b>Typ:</b> %1 &nbsp;|&nbsp; <b>Hersteller/Modell:</b> %2 %3<br>"
        "<b>Achsen:</b> %4 &nbsp;|&nbsp; <b>Traglast:</b> %5 kg &nbsp;|&nbsp; <b>Baujahr:</b> %6<br>"
        "<b>Soll-Taktzeit:</b> <span style='color:#1d4ed8;font-weight:bold;'>%7 s</span> &nbsp;|&nbsp; <b>Status:</b> %8",
        "<b>Type:</b> %1 &nbsp;|&nbsp; <b>Vendor/Model:</b> %2 %3<br>"
        "<b>Axes:</b> %4 &nbsp;|&nbsp; <b>Payload:</b> %5 kg &nbsp;|&nbsp; <b>Build Year:</b> %6<br>"
        "<b>Target Cycle Time:</b> <span style='color:#1d4ed8;font-weight:bold;'>%7 s</span> &nbsp;|&nbsp; <b>Status:</b> %8"
    ).arg(anlagenTypToCsv(a->typ, a->rawTyp), a->hersteller, a->modell)
     .arg(a->achsen).arg(a->traglastKg, 0, 'f', 1).arg(a->baujahr)
     .arg(a->sollTaktzeitS, 0, 'f', 1).arg(anlagenStatusToCsv(a->status, a->rawStatus)));

    const Linie* l = m_repo->findLinie(a->linieId);
    if (l) {
        m_lineInfoLabel->setText(trBi(
            "<b>Fertigungslinie:</b> %1 – %2<br>"
            "<b>Halle:</b> %3 &nbsp;|&nbsp; <b>Position im Materialfluss:</b> %4<br>"
            "<b>Schichtmodell:</b> %5",
            "<b>Production Line:</b> %1 – %2<br>"
            "<b>Hall:</b> %3 &nbsp;|&nbsp; <b>Position in Material Flow:</b> %4<br>"
            "<b>Shift Model:</b> %5"
        ).arg(l->linieId, l->bezeichnung, l->halle).arg(a->position).arg(l->rawSchichtmodell));
    } else {
        m_lineInfoLabel->setText(trBi(
            "<span style='color:#dc2626;font-weight:bold;'>⚠ Verknüpfte Linie '%1' existiert nicht in linien.csv!</span><br>"
            "Position im Materialfluss: %2",
            "<span style='color:#dc2626;font-weight:bold;'>⚠ Linked line '%1' does not exist in linien.csv!</span><br>"
            "Position in material flow: %2"
        ).arg(a->linieId).arg(a->position));
    }

    if (a->hasValidationWarning && !a->validationMessages.isEmpty()) {
        m_warningBanner->setText("⚠ " + trBi("Stammdaten-Plausibilitätshinweis: ", "Master Data Plausibility Note: ")
                                 + a->validationMessages.join(" | "));
        m_warningBanner->show();
    } else {
        m_warningBanner->hide();
    }
}

void AnlageDetailDialog::onDateSelected(int index) {
    if (index < 0) return;
    const QDate date = m_dateCombo->itemData(index).toDate();
    loadSelectedDayLazy(date);
}

void AnlageDetailDialog::loadSelectedDayLazy(const QDate& date) {
    const Anlage* a = m_repo->findAnlage(m_anlageId);
    if (!a) return;

    const QString relPath = m_repo->relativeXmlPath(m_anlageId, date);
    m_xmlPathLabel->setText(relPath.isEmpty()
        ? trBi("Kein Index-Pfad", "No index path")
        : QString("Index-Pfad: %1 (Cache Hits: %2)").arg(relPath).arg(m_repo->cacheHits()));

    auto proto = m_repo->getProtokollLazy(m_anlageId, date);
    m_timelineWidget->setProtokoll(proto);

    // Degradationsverhalten bei fehlendem oder defektem XML (Mindestanforderung 7)
    if (!proto || !proto->loadSuccess) {
        m_degradationBanner->setText(trBi(
            "⚠ Degradationsmodus aktiv – Protokoll für %1 konnte nicht geladen werden: %2\n"
            "Stammdaten, Wartungshistorie und die übrigen Protokolltage stehen weiterhin voll zur Verfügung.",
            "⚠ Graceful Degradation Active – Log for %1 could not be loaded: %2\n"
            "Master data, maintenance history, and other log days remain fully available."
        ).arg(date.toString("dd.MM.yyyy"), proto ? proto->errorMessage : "Unbekannter Fehler"));
        m_degradationBanner->show();

        m_cumPartsWidget->setPoints({});
        m_vBar->setValue(0); m_vBar->setFormat("N/A");
        m_lBar->setValue(0); m_lBar->setFormat("N/A");
        m_qBar->setValue(0); m_qBar->setFormat("N/A");
        m_oeeBar->setValue(0); m_oeeBar->setFormat("N/A");
        m_kpiDetailsLabel->setText(trBi("Keine Tagesauswertung möglich (Protokolldatei fehlt oder beschädigt).",
                                        "No daily evaluation available (log file missing or corrupted)."));
        m_mtbfMttrLabel->clear();
        m_stateDurationTable->setRowCount(0);
        m_shiftCompareTable->setRowCount(0);
        return;
    }

    const OeeMetrics dm = OeeCalculator::calculateDailyOee(*proto, a->sollTaktzeitS);
    m_cumPartsWidget->setPoints(dm.cumulativePoints);

    // Prüfen auf Warnungen im XML oder komplett ungeplanten Tag (z.B. Sonntag)
    if (!proto->warnings.isEmpty()) {
        m_degradationBanner->setStyleSheet("background-color: #fef3c7; border: 1px solid #f59e0b; border-radius: 6px; padding: 8px; color: #92400e; font-weight: bold;");
        m_degradationBanner->setText("⚠ " + trBi("Plausibilitätswarnung im XML-Protokoll erkannt: ", "Plausibility warning in XML log: ")
                                     + proto->warnings.join(" | "));
        m_degradationBanner->show();
    } else if (!dm.hasPlannedShifts) {
        m_degradationBanner->setStyleSheet("background-color: #e0f2fe; border: 1px solid #38bdf8; border-radius: 6px; padding: 8px; color: #0369a1; font-weight: bold;");
        m_degradationBanner->setText(trBi(
            "ℹ An diesem Tag (%1) waren keine Schichten geplant (geplant=\"nein\", z. B. Sonntag).",
            "ℹ No shifts were scheduled on this day (%1, geplant=\"nein\", e.g. Sunday)."
        ).arg(date.toString("dd.MM.yyyy")));
        m_degradationBanner->show();
    } else {
        m_degradationBanner->hide();
    }

    auto setBarPercent = [](QProgressBar* bar, double ratio) {
        const int permille = std::clamp(static_cast<int>(std::round(ratio * 1000.0)), 0, 1000);
        bar->setValue(permille);
        bar->setFormat(QString("%1 %").arg(ratio * 100.0, 0, 'f', 1));
    };

    if (dm.isValid) {
        setBarPercent(m_vBar, dm.verfuegbarkeit);
        setBarPercent(m_lBar, dm.leistungsgrad);
        setBarPercent(m_qBar, dm.qualitaetsrate);
        setBarPercent(m_oeeBar, dm.oee);
    } else {
        m_vBar->setValue(0); m_vBar->setFormat("-");
        m_lBar->setValue(0); m_lBar->setFormat("-");
        m_qBar->setValue(0); m_qBar->setFormat("-");
        m_oeeBar->setValue(0); m_oeeBar->setFormat("-");
    }

    m_kpiDetailsLabel->setText(trBi(
        "<b>Planbelegungszeit (T_plan):</b> %1 Min. &nbsp;|&nbsp; <b>Laufzeit (T_lauf):</b> %2 Min. &nbsp;|&nbsp; "
        "<b>Gutteile:</b> %3 &nbsp;|&nbsp; <b>Ausschuss:</b> %4",
        "<b>Planned Time (T_plan):</b> %1 min &nbsp;|&nbsp; <b>Run Time (T_lauf):</b> %2 min &nbsp;|&nbsp; "
        "<b>Good Parts:</b> %3 &nbsp;|&nbsp; <b>Scrap:</b> %4"
    ).arg(dm.planbelegungszeitSek / 60.0, 0, 'f', 1)
     .arg(dm.laufzeitSek / 60.0, 0, 'f', 1)
     .arg(dm.gutteile).arg(dm.ausschuss));

    m_mtbfMttrLabel->setText(trBi(
        "<b>Zuverlässigkeit (MTBF / MTTR):</b> %1 Störungen &nbsp;|&nbsp; MTBF: %2 Min. &nbsp;|&nbsp; MTTR: %3 Min.",
        "<b>Reliability (MTBF / MTTR):</b> %1 Faults &nbsp;|&nbsp; MTBF: %2 min &nbsp;|&nbsp; MTTR: %3 min"
    ).arg(dm.anzahlStoerungen).arg(dm.mtbfMinuten, 0, 'f', 1).arg(dm.mttrMinuten, 0, 'f', 1));

    // Tabelle 1: Zeitanteile je Zustand füllen
    const QList<ZustandTyp> stateOrder = {
        ZustandTyp::Produktion, ZustandTyp::Ruesten, ZustandTyp::Stoerung,
        ZustandTyp::Materialmangel, ZustandTyp::Rueckstau, ZustandTyp::Wartung,
        ZustandTyp::Pause, ZustandTyp::Unbekannt
    };
    m_stateDurationTable->setRowCount(0);
    for (ZustandTyp zt : stateOrder) {
        const qint64 secs = dm.sekundenJeZustand.value(zt, 0);
        if (zt == ZustandTyp::Unbekannt && secs == 0) continue;
        const int r = m_stateDurationTable->rowCount();
        m_stateDurationTable->insertRow(r);

        auto* nameItem = new QTableWidgetItem(displayNameForZustandTyp(zt));
        nameItem->setBackground(colorForZustandTyp(zt).lighter(165));
        m_stateDurationTable->setItem(r, 0, nameItem);
        m_stateDurationTable->setItem(r, 1, new QTableWidgetItem(QString("%1 min").arg(secs / 60.0, 0, 'f', 1)));
        m_stateDurationTable->setItem(r, 2, new QTableWidgetItem(QString("%1 s").arg(secs)));
        const double share = (dm.gesamtSchichtzeitSek > 0)
            ? (static_cast<double>(secs) / dm.gesamtSchichtzeitSek) * 100.0 : 0.0;
        m_stateDurationTable->setItem(r, 3, new QTableWidgetItem(QString("%1 %").arg(share, 0, 'f', 1)));
    }

    // Tabelle 2: Schichtvergleich (Früh, Spät, Nacht)
    const QList<OeeMetrics> shiftMetrics = OeeCalculator::calculatePerShiftOee(*proto, a->sollTaktzeitS);
    m_shiftCompareTable->setRowCount(shiftMetrics.size());
    for (int i = 0; i < shiftMetrics.size(); ++i) {
        const auto& sm = shiftMetrics[i];
        m_shiftCompareTable->setItem(i, 0, new QTableWidgetItem(sm.label));
        m_shiftCompareTable->setItem(i, 1, new QTableWidgetItem(sm.hasPlannedShifts ? trBi("Ja", "Yes") : trBi("Nein", "No")));
        m_shiftCompareTable->setItem(i, 2, new QTableWidgetItem(sm.isValid ? QString("%1%").arg(sm.verfuegbarkeit * 100.0, 0, 'f', 1) : "-"));
        m_shiftCompareTable->setItem(i, 3, new QTableWidgetItem(sm.isValid ? QString("%1%").arg(sm.leistungsgrad * 100.0, 0, 'f', 1) : "-"));
        m_shiftCompareTable->setItem(i, 4, new QTableWidgetItem(sm.isValid ? QString("%1%").arg(sm.qualitaetsrate * 100.0, 0, 'f', 1) : "-"));
        m_shiftCompareTable->setItem(i, 5, new QTableWidgetItem(sm.isValid ? QString("%1%").arg(sm.oee * 100.0, 0, 'f', 1) : "-"));
        m_shiftCompareTable->setItem(i, 6, new QTableWidgetItem(QString("%1 / %2").arg(sm.gutteile).arg(sm.ausschuss)));
    }
}

void AnlageDetailDialog::populate14DaysTrendTab() {
    const Anlage* a = m_repo->findAnlage(m_anlageId);
    if (!a) return;

    const auto protos = m_repo->loadAllProtokolleForAnlage(m_anlageId);
    const auto [dailyList, anomaly] = OeeCalculator::analyze14DaysTrend(*a, protos);

    m_trendChart->setDailyMetrics(dailyList);

    const QStringList& notes = (LanguageManager::instance().currentLanguage() == AppLanguage::German)
        ? anomaly.summaryNotesDe : anomaly.summaryNotesEn;

    if (anomaly.isConspicuous && !notes.isEmpty()) {
        m_trendAnomalyLabel->setStyleSheet("background-color: #fef3c7; border: 1px solid #f59e0b; border-radius: 5px; padding: 8px; color: #92400e; font-weight: bold;");
        m_trendAnomalyLabel->setText("🔍 " + trBi("Automatische Trend- & Fehleranalyse (14 Tage): ", "Automatic 14-Day Trend & Fault Analysis: ")
                                     + notes.join("  |  ")
                                     + QString("  (Ø OEE: %1%, MTBF: %2 min, MTTR: %3 min)")
                                         .arg(anomaly.avgOee * 100.0, 0, 'f', 1)
                                         .arg(anomaly.totalMtbfMin, 0, 'f', 1)
                                         .arg(anomaly.totalMttrMin, 0, 'f', 1));
    } else {
        m_trendAnomalyLabel->setStyleSheet("background-color: #dcfce7; border: 1px solid #22c55e; border-radius: 5px; padding: 8px; color: #166534; font-weight: bold;");
        m_trendAnomalyLabel->setText(trBi(
            "✓ 14-Tage-Trend unauffällig – Stabile Produktionskennzahlen (Ø OEE: %1%, MTBF: %2 min, MTTR: %3 min).",
            "✓ 14-Day Trend Normal – Stable production metrics (Avg OEE: %1%, MTBF: %2 min, MTTR: %3 min)."
        ).arg(anomaly.avgOee * 100.0, 0, 'f', 1)
         .arg(anomaly.totalMtbfMin, 0, 'f', 1)
         .arg(anomaly.totalMttrMin, 0, 'f', 1));
    }

    m_trendTable->setRowCount(dailyList.size());
    for (int i = 0; i < dailyList.size(); ++i) {
        const auto& dm = dailyList[i];
        m_trendTable->setItem(i, 0, new QTableWidgetItem(dm.datum.toString("yyyy-MM-dd")));
        m_trendTable->setItem(i, 1, new QTableWidgetItem(dm.isValid ? QString("%1%").arg(dm.verfuegbarkeit * 100.0, 0, 'f', 1) : "-"));
        m_trendTable->setItem(i, 2, new QTableWidgetItem(dm.isValid ? QString("%1%").arg(dm.leistungsgrad * 100.0, 0, 'f', 1) : "-"));
        m_trendTable->setItem(i, 3, new QTableWidgetItem(dm.isValid ? QString("%1%").arg(dm.qualitaetsrate * 100.0, 0, 'f', 1) : "-"));
        m_trendTable->setItem(i, 4, new QTableWidgetItem(dm.isValid ? QString("%1%").arg(dm.oee * 100.0, 0, 'f', 1) : "-"));
        m_trendTable->setItem(i, 5, new QTableWidgetItem(QString::number(dm.gutteile)));
        m_trendTable->setItem(i, 6, new QTableWidgetItem(QString::number(dm.ausschuss)));
        m_trendTable->setItem(i, 7, new QTableWidgetItem(QString("%1 (%2 min)").arg(dm.anzahlStoerungen).arg(dm.stoerzeitSek / 60.0, 0, 'f', 1)));
    }
}

void AnlageDetailDialog::populateMaintenanceTab() {
    const QList<Wartung> list = m_repo->wartungenForAnlage(m_anlageId);
    m_maintTable->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const auto& w = list[i];
        m_maintTable->setItem(i, 0, new QTableWidgetItem(w.wartungId));
        m_maintTable->setItem(i, 1, new QTableWidgetItem(w.datum.toString("yyyy-MM-dd")));
        m_maintTable->setItem(i, 2, new QTableWidgetItem(w.art));
        m_maintTable->setItem(i, 3, new QTableWidgetItem(QString("%1 min").arg(w.dauerMin)));
        m_maintTable->setItem(i, 4, new QTableWidgetItem(w.geplant ? trBi("Ja", "Yes") : trBi("Nein", "No")));
        m_maintTable->setItem(i, 5, new QTableWidgetItem(w.bemerkung));
    }
}

void AnlageDetailDialog::onEditMasterDataClicked() {
    const Anlage* a = m_repo->findAnlage(m_anlageId);
    if (!a) return;
    AnlageEditDialog dlg(*a, m_repo, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_repo->updateAnlage(dlg.updatedAnlage());
        refreshMasterDataHeader();
        onDateSelected(m_dateCombo->currentIndex());
        populate14DaysTrendTab();
    }
}

void AnlageDetailDialog::onAddMaintenanceClicked() {
    WartungAddDialog dlg(m_anlageId, m_repo, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_repo->addWartung(dlg.createdWartung());
        populateMaintenanceTab();
        QMessageBox::information(this,
            trBi("Wartung gespeichert", "Maintenance Saved"),
            trBi("Der neue Wartungseintrag wurde im Speicher hinterlegt und kann über 'CSV/SQLite Export' im Hauptfenster exportiert werden.",
                 "The new maintenance entry has been stored in memory and can be exported via 'CSV/SQLite Export' in the main window."));
    }
}

void AnlageDetailDialog::onExportReportCsvClicked() {
    const QString fileName = QFileDialog::getSaveFileName(
        this,
        trBi("14-Tage Anlagenbericht als CSV exportieren", "Export 14-Day Machine Report as CSV"),
        QString("Bericht_%1.csv").arg(m_anlageId),
        "CSV (*.csv)"
    );
    if (fileName.isEmpty()) return;

    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "anlage_id,datum,verfuegbarkeit_pct,leistungsgrad_pct,qualitaetsrate_pct,oee_pct,gutteile,ausschuss,stoerungen,stoerzeit_min,mtbf_min,mttr_min\n";

    const Anlage* a = m_repo->findAnlage(m_anlageId);
    const auto protos = m_repo->loadAllProtokolleForAnlage(m_anlageId);
    for (const auto& p : protos) {
        if (!p) continue;
        const OeeMetrics dm = OeeCalculator::calculateDailyOee(*p, a ? a->sollTaktzeitS : 45.0);
        out << m_anlageId << ","
            << dm.datum.toString("yyyy-MM-dd") << ","
            << QString::number(dm.verfuegbarkeit * 100.0, 'f', 2) << ","
            << QString::number(dm.leistungsgrad * 100.0, 'f', 2) << ","
            << QString::number(dm.qualitaetsrate * 100.0, 'f', 2) << ","
            << QString::number(dm.oee * 100.0, 'f', 2) << ","
            << dm.gutteile << ","
            << dm.ausschuss << ","
            << dm.anzahlStoerungen << ","
            << QString::number(dm.stoerzeitSek / 60.0, 'f', 1) << ","
            << QString::number(dm.mtbfMinuten, 'f', 1) << ","
            << QString::number(dm.mttrMinuten, 'f', 1) << "\n";
    }
    QMessageBox::information(this, trBi("CSV-Bericht exportiert", "CSV Report Exported"),
                             trBi("Bericht erfolgreich gespeichert unter:\n%1", "Report saved to:\n%1").arg(fileName));
}

void AnlageDetailDialog::onExportReportPdfClicked() {
    const QString fileName = QFileDialog::getSaveFileName(
        this,
        trBi("Anlagenbericht als PDF exportieren", "Export Machine Report as PDF"),
        QString("Bericht_%1.pdf").arg(m_anlageId),
        "PDF (*.pdf)"
    );
    if (fileName.isEmpty()) return;

    const Anlage* a = m_repo->findAnlage(m_anlageId);
    if (!a) return;

    const auto protos = m_repo->loadAllProtokolleForAnlage(m_anlageId);
    const auto [dailyList, anomaly] = OeeCalculator::analyze14DaysTrend(*a, protos);

    QString html;
    html += QString("<h2>RoboCell – 14-Tage OEE- & Zuverlässigkeitsbericht: %1 (%2)</h2>").arg(a->anlageId, a->bezeichnung);
    html += QString("<p><b>Linie:</b> %1 (Pos. %2) | <b>Typ:</b> %3 | <b>Hersteller/Modell:</b> %4 %5 | <b>Soll-Taktzeit:</b> %6 s</p>")
                .arg(a->linieId).arg(a->position).arg(anlagenTypToCsv(a->typ, a->rawTyp))
                .arg(a->hersteller, a->modell).arg(a->sollTaktzeitS, 0, 'f', 1);
    html += QString("<p><b>14-Tage Durchschnitt:</b> OEE = <b>%1%</b> (V = %2%, L = %3%, Q = %4%) | MTBF = %5 min | MTTR = %6 min</p>")
                .arg(anomaly.avgOee * 100.0, 0, 'f', 1)
                .arg(anomaly.avgVerfuegbarkeit * 100.0, 0, 'f', 1)
                .arg(anomaly.avgLeistungsgrad * 100.0, 0, 'f', 1)
                .arg(anomaly.avgQualitaet * 100.0, 0, 'f', 1)
                .arg(anomaly.totalMtbfMin, 0, 'f', 1)
                .arg(anomaly.totalMttrMin, 0, 'f', 1);

    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%' style='font-size:9pt;'>";
    html += "<tr style='background-color:#e2e8f0;'><th>Datum</th><th>V [%]</th><th>L [%]</th><th>Q [%]</th><th>OEE [%]</th><th>Gutteile</th><th>Ausschuss</th><th>Störzeit [min]</th></tr>";
    for (const auto& dm : dailyList) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td><b>%5</b></td><td>%6</td><td>%7</td><td>%8</td></tr>")
                    .arg(dm.datum.toString("yyyy-MM-dd"))
                    .arg(dm.isValid ? QString::number(dm.verfuegbarkeit * 100.0, 'f', 1) : "-")
                    .arg(dm.isValid ? QString::number(dm.leistungsgrad * 100.0, 'f', 1) : "-")
                    .arg(dm.isValid ? QString::number(dm.qualitaetsrate * 100.0, 'f', 1) : "-")
                    .arg(dm.isValid ? QString::number(dm.oee * 100.0, 'f', 1) : "-")
                    .arg(dm.gutteile)
                    .arg(dm.ausschuss)
                    .arg(dm.stoerzeitSek / 60.0, 0, 'f', 1);
    }
    html += "</table>";

    QTextDocument doc;
    doc.setHtml(html);
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    doc.print(&printer);

    QMessageBox::information(this, trBi("PDF-Bericht erstellt", "PDF Report Created"),
                             trBi("PDF-Bericht erfolgreich gespeichert unter:\n%1", "PDF report saved to:\n%1").arg(fileName));
}

} // namespace RoboCell
