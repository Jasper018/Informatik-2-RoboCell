#include "mainwindow.h"
#include "anlagedetaildialog.h"
#include "anlagenmodell.h"
#include "anmeldedialog.h"
#include "bearbeitendialoge.h"
#include "einstellungendialog.h"
#include "fensterhilfe.h"
#include "linienansicht.h"
#include "core/benutzerverwaltung.h"
#include "core/einstellungen.h"
#include "core/farben.h"
#include "core/sprache.h"
#include "io/datenverwaltung.h"
#include "logic/oeeberechnung.h"
#include <QAbstractItemView>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QTableView>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

MainWindow::MainWindow(Datenverwaltung* daten, Benutzerverwaltung* benutzer, QWidget* parent)
    : QMainWindow(parent)
    , m_daten(daten)
    , m_benutzer(benutzer)
{
    setMinimumSize(640, 480);

    menueLeisteBauen();

    QWidget* zentral = new QWidget(this);
    setCentralWidget(zentral);
    QVBoxLayout* haupt = new QVBoxLayout(zentral);
    haupt->setSpacing(6);
    haupt->setContentsMargins(10, 8, 10, 6);

    // Automatischer Flag-Hinweis direkt unter der Menueleiste
    m_flagLeiste = new QWidget(zentral);
    QHBoxLayout* flagLeisteLayout = new QHBoxLayout(m_flagLeiste);
    flagLeisteLayout->setContentsMargins(0, 0, 0, 0);
    flagLeisteLayout->setSpacing(8);
    m_flagLeisteText = new QLabel(m_flagLeiste);
    m_flagLeisteText->setWordWrap(true);
    m_flagLeisteBtn = new QPushButton(m_flagLeiste);
    flagLeisteLayout->addWidget(m_flagLeisteText, 1);
    flagLeisteLayout->addWidget(m_flagLeisteBtn);
    m_flagLeiste->hide();
    haupt->addWidget(m_flagLeiste);

    // 5 Arbeitsbereiche in Registerkarten
    m_tabs = new QTabWidget(zentral);
    haupt->addWidget(m_tabs, 1);

    // -----------------------------------------------------------------------
    // Tab 1: Anlagenliste und Filter
    // -----------------------------------------------------------------------
    QWidget* tabAnlagen = new QWidget(m_tabs);
    QVBoxLayout* anlagenLayout = new QVBoxLayout(tabAnlagen);

    m_filterGruppe = new QGroupBox(tabAnlagen);
    QGridLayout* filterGrid = new QGridLayout(m_filterGruppe);
    filterGrid->setSpacing(6);

    m_sucheLabel = new QLabel(m_filterGruppe);
    m_sucheEdit = new QLineEdit(m_filterGruppe);
    m_sucheEdit->setClearButtonEnabled(true);

    m_linieLabel = new QLabel(m_filterGruppe);
    m_linieCombo = new QComboBox(m_filterGruppe);

    m_typLabel = new QLabel(m_filterGruppe);
    m_typCombo = new QComboBox(m_filterGruppe);

    m_statusLabel = new QLabel(m_filterGruppe);
    m_statusCombo = new QComboBox(m_filterGruppe);

    m_baujahrLabel = new QLabel(m_filterGruppe);
    m_baujahrSpin = new QSpinBox(m_filterGruppe);
    m_baujahrSpin->setRange(2000, 2040);
    m_baujahrSpin->setValue(2000);

    m_auffaelligCheck = new QCheckBox(m_filterGruppe);
    m_flagCheck = new QCheckBox(m_filterGruppe);
    m_filterResetBtn = new QPushButton(m_filterGruppe);

    filterGrid->addWidget(m_sucheLabel, 0, 0);
    filterGrid->addWidget(m_sucheEdit, 0, 1);
    filterGrid->addWidget(m_linieLabel, 0, 2);
    filterGrid->addWidget(m_linieCombo, 0, 3);
    filterGrid->addWidget(m_typLabel, 0, 4);
    filterGrid->addWidget(m_typCombo, 0, 5);
    filterGrid->addWidget(m_statusLabel, 0, 6);
    filterGrid->addWidget(m_statusCombo, 0, 7);

    filterGrid->addWidget(m_baujahrLabel, 1, 0);
    filterGrid->addWidget(m_baujahrSpin, 1, 1);
    filterGrid->addWidget(m_auffaelligCheck, 1, 2, 1, 2);
    filterGrid->addWidget(m_flagCheck, 1, 4, 1, 2);
    filterGrid->addWidget(m_filterResetBtn, 1, 6, 1, 2);
    filterGrid->setColumnStretch(1, 2);
    filterGrid->setColumnStretch(3, 1);
    filterGrid->setColumnStretch(5, 1);
    filterGrid->setColumnStretch(7, 1);
    anlagenLayout->addWidget(m_filterGruppe);

    m_anlagenModell = new AnlagenModell(m_daten, this);
    m_filterModell = new AnlagenFilterModell(this);
    m_filterModell->setSourceModel(m_anlagenModell);

    auto tabelleVorbereiten = [](QTableView* view) {
        view->setAlternatingRowColors(true);
        view->setWordWrap(false);
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->horizontalHeader()->setStretchLastSection(true);
        view->verticalHeader()->setVisible(false);
        view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        view->verticalHeader()->setDefaultSectionSize(28);
    };

    m_anlagenView = new QTableView(tabAnlagen);
    m_anlagenView->setModel(m_filterModell);
    m_anlagenView->setSortingEnabled(true);
    m_anlagenView->sortByColumn(AnlagenModell::SpAnlageId, Qt::AscendingOrder);
    m_anlagenView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_anlagenView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_anlagenView->setContextMenuPolicy(Qt::CustomContextMenu);
    tabelleVorbereiten(m_anlagenView);
    anlagenLayout->addWidget(m_anlagenView, 1);

    QHBoxLayout* fussLeiste = new QHBoxLayout();
    m_anzahlLabel = new QLabel(tabAnlagen);
    m_detailBtn = new QPushButton(tabAnlagen);
    m_bearbeitenBtn = new QPushButton(tabAnlagen);
    m_wartungBtn = new QPushButton(tabAnlagen);
    fussLeiste->addWidget(m_anzahlLabel);
    fussLeiste->addStretch();
    fussLeiste->addWidget(m_detailBtn);
    fussLeiste->addWidget(m_bearbeitenBtn);
    fussLeiste->addWidget(m_wartungBtn);
    anlagenLayout->addLayout(fussLeiste);

    m_tabs->addTab(tabAnlagen, QString());

    // -----------------------------------------------------------------------
    // Tab 2: Grafische Linienansicht und Engpassanalyse
    // -----------------------------------------------------------------------
    m_linienAnsicht = new LinienAnsichtWidget(m_daten, m_tabs);
    m_tabs->addTab(m_linienAnsicht, QString());

    // -----------------------------------------------------------------------
    // Tab 3: Stoercodes und Pareto-Statistik
    // -----------------------------------------------------------------------
    QWidget* tabPareto = new QWidget(m_tabs);
    QVBoxLayout* paretoLayout = new QVBoxLayout(tabPareto);
    QHBoxLayout* paretoFilterLeiste = new QHBoxLayout();
    m_paretoFilterLabel = new QLabel(tabPareto);
    m_paretoLinieCombo = new QComboBox(tabPareto);
    paretoFilterLeiste->addWidget(m_paretoFilterLabel);
    paretoFilterLeiste->addWidget(m_paretoLinieCombo);
    paretoFilterLeiste->addStretch();
    paretoLayout->addLayout(paretoFilterLeiste);

    QSplitter* paretoTeiler = new QSplitter(Qt::Horizontal, tabPareto);
    paretoTeiler->setChildrenCollapsible(false);

    m_stoercodeTabelle = new QTableWidget(paretoTeiler);
    m_stoercodeTabelle->setColumnCount(3);
    m_stoercodeTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabelleVorbereiten(m_stoercodeTabelle);

    m_paretoTabelle = new QTableWidget(paretoTeiler);
    m_paretoTabelle->setColumnCount(6);
    m_paretoTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabelleVorbereiten(m_paretoTabelle);

    paretoTeiler->addWidget(m_stoercodeTabelle);
    paretoTeiler->addWidget(m_paretoTabelle);
    paretoTeiler->setStretchFactor(0, 2);
    paretoTeiler->setStretchFactor(1, 3);
    paretoLayout->addWidget(paretoTeiler, 1);
    m_tabs->addTab(tabPareto, QString());

    // -----------------------------------------------------------------------
    // Tab 4: Linien, Produkte und Wartungen
    // -----------------------------------------------------------------------
    QWidget* tabStamm = new QWidget(m_tabs);
    QVBoxLayout* stammLayout = new QVBoxLayout(tabStamm);
    QSplitter* stammTeiler = new QSplitter(Qt::Vertical, tabStamm);
    stammTeiler->setChildrenCollapsible(false);

    QWidget* obenBox = new QWidget(stammTeiler);
    QHBoxLayout* obenLayout = new QHBoxLayout(obenBox);
    obenLayout->setContentsMargins(0, 0, 0, 0);

    QWidget* linienBox = new QWidget(obenBox);
    QVBoxLayout* linienBoxLayout = new QVBoxLayout(linienBox);
    linienBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_linienUeberschrift = new QLabel(linienBox);
    m_linienTabelle = new QTableWidget(linienBox);
    m_linienTabelle->setColumnCount(6);
    m_linienTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabelleVorbereiten(m_linienTabelle);
    linienBoxLayout->addWidget(m_linienUeberschrift);
    linienBoxLayout->addWidget(m_linienTabelle, 1);

    QWidget* prodBox = new QWidget(obenBox);
    QVBoxLayout* prodBoxLayout = new QVBoxLayout(prodBox);
    prodBoxLayout->setContentsMargins(0, 0, 0, 0);
    m_produkteUeberschrift = new QLabel(prodBox);
    m_produkteTabelle = new QTableWidget(prodBox);
    m_produkteTabelle->setColumnCount(4);
    m_produkteTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabelleVorbereiten(m_produkteTabelle);
    prodBoxLayout->addWidget(m_produkteUeberschrift);
    prodBoxLayout->addWidget(m_produkteTabelle, 1);

    obenLayout->addWidget(linienBox, 3);
    obenLayout->addWidget(prodBox, 2);
    stammTeiler->addWidget(obenBox);

    QWidget* wartBox = new QWidget(stammTeiler);
    QVBoxLayout* wartBoxLayout = new QVBoxLayout(wartBox);
    wartBoxLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* wartKopf = new QHBoxLayout();
    m_wartungenUeberschrift = new QLabel(wartBox);
    m_neueWartungTabBtn = new QPushButton(wartBox);
    wartKopf->addWidget(m_wartungenUeberschrift);
    wartKopf->addStretch();
    wartKopf->addWidget(m_neueWartungTabBtn);
    wartBoxLayout->addLayout(wartKopf);

    m_wartungenTabelle = new QTableWidget(wartBox);
    m_wartungenTabelle->setColumnCount(7);
    m_wartungenTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabelleVorbereiten(m_wartungenTabelle);
    wartBoxLayout->addWidget(m_wartungenTabelle, 1);
    stammTeiler->addWidget(wartBox);

    stammLayout->addWidget(stammTeiler, 1);
    m_tabs->addTab(tabStamm, QString());

    // -----------------------------------------------------------------------
    // Tab 5: Datenpruefung (automatische Flags)
    // -----------------------------------------------------------------------
    QWidget* tabFlags = new QWidget(m_tabs);
    QVBoxLayout* flagsLayout = new QVBoxLayout(tabFlags);
    m_flagZusammenfassung = new QLabel(tabFlags);
    m_flagZusammenfassung->setWordWrap(true);
    m_flagZusammenfassung->setProperty("hinweis", true);
    flagsLayout->addWidget(m_flagZusammenfassung);

    QHBoxLayout* flagFilterZeile = new QHBoxLayout();
    m_flagGradLabel = new QLabel(tabFlags);
    m_flagGradCombo = new QComboBox(tabFlags);
    m_flagDateiLabel = new QLabel(tabFlags);
    m_flagDateiCombo = new QComboBox(tabFlags);
    m_flagSucheLabel = new QLabel(tabFlags);
    m_flagSucheEdit = new QLineEdit(tabFlags);
    m_flagSucheEdit->setClearButtonEnabled(true);
    flagFilterZeile->addWidget(m_flagGradLabel);
    flagFilterZeile->addWidget(m_flagGradCombo);
    flagFilterZeile->addSpacing(8);
    flagFilterZeile->addWidget(m_flagDateiLabel);
    flagFilterZeile->addWidget(m_flagDateiCombo);
    flagFilterZeile->addSpacing(8);
    flagFilterZeile->addWidget(m_flagSucheLabel);
    flagFilterZeile->addWidget(m_flagSucheEdit, 1);
    flagsLayout->addLayout(flagFilterZeile);

    m_flagTabelle = new QTableWidget(tabFlags);
    m_flagTabelle->setColumnCount(5);
    m_flagTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_flagTabelle->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabelleVorbereiten(m_flagTabelle);
    flagsLayout->addWidget(m_flagTabelle, 1);
    m_tabs->addTab(tabFlags, QString());

    // -----------------------------------------------------------------------
    // Statusleiste
    // -----------------------------------------------------------------------
    m_statusOrdner = new QLabel(this);
    m_statusBenutzer = new QLabel(this);
    m_statusFlags = new QLabel(this);
    m_statusCache = new QLabel(this);
    statusBar()->addWidget(m_statusOrdner, 1);
    statusBar()->addPermanentWidget(m_statusBenutzer);
    statusBar()->addPermanentWidget(m_statusFlags);
    statusBar()->addPermanentWidget(m_statusCache);

    // -----------------------------------------------------------------------
    // Signals und Slots verbinden
    // -----------------------------------------------------------------------
    connect(m_flagLeisteBtn, &QPushButton::clicked, this, &MainWindow::slotFlagsTabZeigen);

    connect(m_sucheEdit, &QLineEdit::textChanged, this, &MainWindow::slotFilterGeaendert);
    connect(m_linieCombo, &QComboBox::currentIndexChanged, this, &MainWindow::slotFilterGeaendert);
    connect(m_typCombo, &QComboBox::currentIndexChanged, this, &MainWindow::slotFilterGeaendert);
    connect(m_statusCombo, &QComboBox::currentIndexChanged, this, &MainWindow::slotFilterGeaendert);
    connect(m_baujahrSpin, &QSpinBox::valueChanged, this, &MainWindow::slotFilterGeaendert);
    connect(m_auffaelligCheck, &QCheckBox::toggled, this, &MainWindow::slotFilterGeaendert);
    connect(m_flagCheck, &QCheckBox::toggled, this, &MainWindow::slotFilterGeaendert);
    connect(m_filterResetBtn, &QPushButton::clicked, this, &MainWindow::slotFilterZuruecksetzen);

    connect(m_anlagenView, &QTableView::doubleClicked, this, &MainWindow::slotAnlageDoppelklick);
    connect(m_anlagenView, &QTableView::customContextMenuRequested, this, &MainWindow::slotAnlageKontextmenue);
    connect(m_detailBtn, &QPushButton::clicked, this, &MainWindow::slotAusgewaehlteAnlageOeffnen);
    connect(m_bearbeitenBtn, &QPushButton::clicked, this, &MainWindow::slotAnlageBearbeiten);
    connect(m_wartungBtn, &QPushButton::clicked, this, &MainWindow::slotWartungErfassen);
    connect(m_neueWartungTabBtn, &QPushButton::clicked, this, &MainWindow::slotWartungErfassen);

    connect(m_linienAnsicht, &LinienAnsichtWidget::anlageDoppelklick,
            this, [this](const QString& id) { anlageDetailOeffnen(id); });
    connect(m_paretoLinieCombo, &QComboBox::currentIndexChanged, this, &MainWindow::slotParetoAktualisieren);

    connect(m_flagGradCombo, &QComboBox::currentIndexChanged, this, &MainWindow::slotFlagFilterGeaendert);
    connect(m_flagDateiCombo, &QComboBox::currentIndexChanged, this, &MainWindow::slotFlagFilterGeaendert);
    connect(m_flagSucheEdit, &QLineEdit::textChanged, this, &MainWindow::slotFlagFilterGeaendert);
    connect(m_flagTabelle, &QTableWidget::cellDoubleClicked, this, &MainWindow::slotFlagDoppelklick);

    connect(m_daten, &Datenverwaltung::datensatzGeladen, this, &MainWindow::slotDatenGeaendert);
    connect(m_daten, &Datenverwaltung::pruefFortschritt, this, [this](int aktuell, int gesamt) {
        m_statusFlags->setText(tr("Protokollprüfung: %1 / %2").arg(aktuell).arg(gesamt));
    });
    connect(m_daten, &Datenverwaltung::anlageGeaendert, this, &MainWindow::slotDatenGeaendert);
    connect(m_daten, &Datenverwaltung::wartungHinzugefuegt, this, &MainWindow::slotDatenGeaendert);
    connect(m_benutzer, &Benutzerverwaltung::benutzerGeaendert, this, &MainWindow::slotRollenAktualisieren);

    texteAktualisieren();
    allesFuellen();
    slotRollenAktualisieren();
    slotSpaltenAnpassen();

    Fensterhilfe::geometrieSicherWiederherstellen(this, Einstellungen::fensterGeometrie(), 1240, 800);
}

void MainWindow::menueLeisteBauen()
{
    QMenuBar* bar = menuBar();

    // Datei
    m_dateiMenu = bar->addMenu(QString());
    m_actOrdner = m_dateiMenu->addAction(QString());
    m_actOrdner->setShortcut(QKeySequence::Open);
    m_actNeuLaden = m_dateiMenu->addAction(QString());
    m_actNeuLaden->setShortcut(QKeySequence::Refresh);
    m_dateiMenu->addSeparator();
    m_actSqliteLaden = m_dateiMenu->addAction(QString());
    m_actSqliteSpeichern = m_dateiMenu->addAction(QString());
    m_actCsvExport = m_dateiMenu->addAction(QString());
    m_actCsvExport->setShortcut(QKeySequence("Ctrl+E"));
    m_dateiMenu->addSeparator();
    m_actBeenden = m_dateiMenu->addAction(QString());
    m_actBeenden->setShortcut(QKeySequence::Quit);

    // Bearbeiten
    m_bearbeitenMenu = bar->addMenu(QString());
    m_actDetails = m_bearbeitenMenu->addAction(QString());
    m_actBearbeiten = m_bearbeitenMenu->addAction(QString());
    m_actWartung = m_bearbeitenMenu->addAction(QString());

    // Ansicht
    m_ansichtMenu = bar->addMenu(QString());
    m_actTabAnlagen = m_ansichtMenu->addAction(QString());
    m_actTabAnlagen->setShortcut(QKeySequence("Ctrl+1"));
    m_actTabLinien = m_ansichtMenu->addAction(QString());
    m_actTabLinien->setShortcut(QKeySequence("Ctrl+2"));
    m_actTabPareto = m_ansichtMenu->addAction(QString());
    m_actTabPareto->setShortcut(QKeySequence("Ctrl+3"));
    m_actTabStamm = m_ansichtMenu->addAction(QString());
    m_actTabStamm->setShortcut(QKeySequence("Ctrl+4"));
    m_actTabFlags = m_ansichtMenu->addAction(QString());
    m_actTabFlags->setShortcut(QKeySequence("Ctrl+5"));
    m_ansichtMenu->addSeparator();
    m_actFilterReset = m_ansichtMenu->addAction(QString());
    m_actFilterReset->setShortcut(QKeySequence("Ctrl+R"));
    m_actSpaltenAnpassen = m_ansichtMenu->addAction(QString());

    // Einstellungen
    m_einstellungenMenu = bar->addMenu(QString());
    m_actEinstellungen = m_einstellungenMenu->addAction(QString());
    m_actEinstellungen->setShortcut(QKeySequence("Ctrl+,"));
    m_spracheUntermenu = m_einstellungenMenu->addMenu(QString());
    m_spracheGruppe = new QActionGroup(this);
    m_actDeutsch = m_spracheUntermenu->addAction("Deutsch");
    m_actDeutsch->setCheckable(true);
    m_actEnglisch = m_spracheUntermenu->addAction("English");
    m_actEnglisch->setCheckable(true);
    m_spracheGruppe->addAction(m_actDeutsch);
    m_spracheGruppe->addAction(m_actEnglisch);

    // Konto
    m_kontoMenu = bar->addMenu(QString());
    m_actKontoInfo = m_kontoMenu->addAction(QString());
    m_actKontoInfo->setEnabled(false);
    m_actRechte = m_kontoMenu->addAction(QString());
    m_kontoMenu->addSeparator();
    m_actBenutzerWechseln = m_kontoMenu->addAction(QString());
    m_actBenutzerWechseln->setShortcut(QKeySequence("Ctrl+L"));
    m_actAbmelden = m_kontoMenu->addAction(QString());

    // Hilfe
    m_hilfeMenu = bar->addMenu(QString());
    m_actOeeHilfe = m_hilfeMenu->addAction(QString());
    m_actUeber = m_hilfeMenu->addAction(QString());

    connect(m_actOrdner, &QAction::triggered, this, &MainWindow::slotOrdnerOeffnen);
    connect(m_actNeuLaden, &QAction::triggered, this, &MainWindow::slotNeuLaden);
    connect(m_actSqliteLaden, &QAction::triggered, this, &MainWindow::slotSqliteLaden);
    connect(m_actSqliteSpeichern, &QAction::triggered, this, &MainWindow::slotSqliteSpeichern);
    connect(m_actCsvExport, &QAction::triggered, this, &MainWindow::slotCsvExportieren);
    connect(m_actBeenden, &QAction::triggered, this, &QWidget::close);

    connect(m_actDetails, &QAction::triggered, this, &MainWindow::slotAusgewaehlteAnlageOeffnen);
    connect(m_actBearbeiten, &QAction::triggered, this, &MainWindow::slotAnlageBearbeiten);
    connect(m_actWartung, &QAction::triggered, this, &MainWindow::slotWartungErfassen);

    connect(m_actTabAnlagen, &QAction::triggered, this, [this]() { m_tabs->setCurrentIndex(0); });
    connect(m_actTabLinien, &QAction::triggered, this, [this]() { m_tabs->setCurrentIndex(1); });
    connect(m_actTabPareto, &QAction::triggered, this, [this]() { m_tabs->setCurrentIndex(2); });
    connect(m_actTabStamm, &QAction::triggered, this, [this]() { m_tabs->setCurrentIndex(3); });
    connect(m_actTabFlags, &QAction::triggered, this, [this]() { m_tabs->setCurrentIndex(4); });
    connect(m_actFilterReset, &QAction::triggered, this, &MainWindow::slotFilterZuruecksetzen);
    connect(m_actSpaltenAnpassen, &QAction::triggered, this, &MainWindow::slotSpaltenAnpassen);

    connect(m_actEinstellungen, &QAction::triggered, this, &MainWindow::slotEinstellungenOeffnen);
    connect(m_actDeutsch, &QAction::triggered, this, &MainWindow::slotSpracheDeutsch);
    connect(m_actEnglisch, &QAction::triggered, this, &MainWindow::slotSpracheEnglisch);

    connect(m_actRechte, &QAction::triggered, this, &MainWindow::slotBerechtigungenZeigen);
    connect(m_actBenutzerWechseln, &QAction::triggered, this, &MainWindow::slotBenutzerWechseln);
    connect(m_actAbmelden, &QAction::triggered, this, &MainWindow::slotAbmelden);

    connect(m_actOeeHilfe, &QAction::triggered, this, &MainWindow::slotOeeHilfe);
    connect(m_actUeber, &QAction::triggered, this, &MainWindow::slotUeberProgramm);
}

void MainWindow::changeEvent(QEvent* ereignis)
{
    if (ereignis->type() == QEvent::LanguageChange) {
        texteAktualisieren();
        m_anlagenModell->spracheAktualisieren();
        allesFuellen();
        slotRollenAktualisieren();
    }
    QMainWindow::changeEvent(ereignis);
}

void MainWindow::closeEvent(QCloseEvent* ereignis)
{
    Einstellungen::setFensterGeometrie(saveGeometry());
    QMainWindow::closeEvent(ereignis);
}

void MainWindow::texteAktualisieren()
{
    setWindowTitle(tr("RoboCell - Fertigungskennzahlen und OEE"));

    m_dateiMenu->setTitle(tr("Datei"));
    m_actOrdner->setText(tr("Datenordner öffnen..."));
    m_actNeuLaden->setText(tr("Datensatz neu laden und prüfen"));
    m_actSqliteLaden->setText(tr("Aus SQLite-Datenbank laden..."));
    m_actSqliteSpeichern->setText(tr("In SQLite-Datenbank speichern..."));
    m_actCsvExport->setText(tr("Stammdaten als CSV exportieren..."));
    m_actBeenden->setText(tr("Beenden"));

    m_bearbeitenMenu->setTitle(tr("Bearbeiten"));
    m_actDetails->setText(tr("Anlagendetails öffnen"));
    m_actBearbeiten->setText(tr("Stammdaten bearbeiten..."));
    m_actWartung->setText(tr("Neue Wartung erfassen..."));

    m_ansichtMenu->setTitle(tr("Ansicht"));
    m_actTabAnlagen->setText(tr("Anlagen"));
    m_actTabLinien->setText(tr("Linienansicht und Engpass"));
    m_actTabPareto->setText(tr("Störcodes und Pareto"));
    m_actTabStamm->setText(tr("Stammdaten"));
    m_actTabFlags->setText(tr("Datenprüfung (Flags)"));
    m_actFilterReset->setText(tr("Filter zurücksetzen"));
    m_actSpaltenAnpassen->setText(tr("Spaltenbreite anpassen"));

    m_einstellungenMenu->setTitle(tr("Einstellungen"));
    m_actEinstellungen->setText(tr("Einstellungen..."));
    m_spracheUntermenu->setTitle(tr("Sprache"));
    m_actDeutsch->setChecked(Sprache::aktuell() == "de");
    m_actEnglisch->setChecked(Sprache::aktuell() == "en");

    m_kontoMenu->setTitle(tr("Konto"));
    m_actRechte->setText(tr("Berechtigungen anzeigen"));
    m_actBenutzerWechseln->setText(tr("Benutzer wechseln..."));
    m_actAbmelden->setText(tr("Abmelden"));

    m_hilfeMenu->setTitle(tr("Hilfe"));
    m_actOeeHilfe->setText(tr("OEE-Formeln und Kennzahlen"));
    m_actUeber->setText(tr("Über RoboCell"));

    m_flagLeisteBtn->setText(tr("Flags anzeigen"));

    m_tabs->setTabText(0, tr("Anlagen (%1)").arg(m_daten->anlagen().size()));
    m_tabs->setTabText(1, tr("Linienansicht und Engpass"));
    m_tabs->setTabText(2, tr("Störcodes und Pareto"));
    m_tabs->setTabText(3, tr("Stammdaten"));
    m_tabs->setTabText(4, tr("Datenprüfung (%1 Flags)").arg(m_daten->alleProbleme().size()));

    m_filterGruppe->setTitle(tr("Suche und Filter"));
    m_sucheLabel->setText(tr("Suche (ID, Bez., Linie):"));
    m_sucheEdit->setPlaceholderText(tr("z. B. A025, Schweißroboter, L2"));
    m_linieLabel->setText(tr("Linie:"));
    m_typLabel->setText(tr("Typ:"));
    m_statusLabel->setText(tr("Status:"));
    m_baujahrLabel->setText(tr("Baujahr max.:"));
    m_baujahrSpin->setSpecialValueText(tr("Alle"));
    m_auffaelligCheck->setText(tr("Nur auffällige Anlagen (14T-Trend)"));
    m_flagCheck->setText(tr("Nur Anlagen mit Flag"));
    m_filterResetBtn->setText(tr("Filter zurücksetzen"));

    m_detailBtn->setText(tr("Details öffnen"));
    m_bearbeitenBtn->setText(tr("Stammdaten bearbeiten"));
    m_wartungBtn->setText(tr("Wartung erfassen"));

    // Typ-Auswahl
    const int alterTyp = m_typCombo->currentIndex();
    m_typCombo->blockSignals(true);
    m_typCombo->clear();
    m_typCombo->addItem(tr("Alle Typen"), -1);
    const AnlagenTyp typen[8] = {
        AnlagenTyp::CncBeladung, AnlagenTyp::Schweissroboter, AnlagenTyp::Montageroboter,
        AnlagenTyp::Schraubstation, AnlagenTyp::Klebestation, AnlagenTyp::Pruefstation,
        AnlagenTyp::Laserbeschriftung, AnlagenTyp::Palettierer
    };
    for (int i = 0; i < 8; i++) {
        m_typCombo->addItem(anlagenTypName(typen[i]), int(typen[i]));
    }
    if (alterTyp >= 0 && alterTyp < m_typCombo->count()) m_typCombo->setCurrentIndex(alterTyp);
    m_typCombo->blockSignals(false);

    // Status-Auswahl
    const int alterStatus = m_statusCombo->currentIndex();
    m_statusCombo->blockSignals(true);
    m_statusCombo->clear();
    m_statusCombo->addItem(tr("Alle Status"), -1);
    const AnlagenStatus stati[4] = {
        AnlagenStatus::InBetrieb, AnlagenStatus::Stoerung,
        AnlagenStatus::Wartung, AnlagenStatus::Umbau
    };
    for (int i = 0; i < 4; i++) {
        m_statusCombo->addItem(anlagenStatusName(stati[i]), int(stati[i]));
    }
    if (alterStatus >= 0 && alterStatus < m_statusCombo->count()) m_statusCombo->setCurrentIndex(alterStatus);
    m_statusCombo->blockSignals(false);

    m_paretoFilterLabel->setText(tr("Pareto-Auswertung nach Linie filtern:"));
    m_stoercodeTabelle->setHorizontalHeaderLabels({
        tr("Code"), tr("Kategorie"), tr("Beschreibung")
    });
    m_paretoTabelle->setHorizontalHeaderLabels({
        tr("Rang"), tr("Code"), tr("Kategorie"), tr("Beschreibung"),
        tr("Häufigkeit"), tr("Dauer (kumuliert)")
    });

    m_linienUeberschrift->setText(tr("Fertigungslinien (linien.csv)"));
    m_produkteUeberschrift->setText(tr("Produkte (produkte.csv)"));
    m_wartungenUeberschrift->setText(tr("Wartungen (wartungen.csv)"));
    m_neueWartungTabBtn->setText(tr("Neue Wartung erfassen"));

    m_linienTabelle->setHorizontalHeaderLabels({
        tr("Linie"), tr("Bezeichnung"), tr("Halle"),
        tr("Schichtmodell"), tr("Anlagen"), tr("Ø Linien-OEE")
    });
    m_produkteTabelle->setHorizontalHeaderLabels({
        tr("Produkt-ID"), tr("Linie"), tr("Bezeichnung"), tr("Prüfung")
    });
    m_wartungenTabelle->setHorizontalHeaderLabels({
        tr("Wartung-ID"), tr("Anlage"), tr("Datum"), tr("Art"),
        tr("Dauer [min]"), tr("Geplant"), tr("Bemerkung / Flag")
    });

    m_flagGradLabel->setText(tr("Schweregrad:"));
    m_flagDateiLabel->setText(tr("Datei:"));
    m_flagSucheLabel->setText(tr("Suche:"));
    m_flagSucheEdit->setPlaceholderText(tr("ID oder Meldung filtern..."));

    const int alterGrad = std::max(0, m_flagGradCombo->currentIndex());
    m_flagGradCombo->blockSignals(true);
    m_flagGradCombo->clear();
    m_flagGradCombo->addItem(tr("Alle"), -1);
    m_flagGradCombo->addItem(tr("Fehler"), int(Schweregrad::Fehler));
    m_flagGradCombo->addItem(tr("Warnung"), int(Schweregrad::Warnung));
    m_flagGradCombo->setCurrentIndex(alterGrad);
    m_flagGradCombo->blockSignals(false);

    m_flagTabelle->setHorizontalHeaderLabels({
        tr("Schweregrad"), tr("Datei"), tr("Zeile"), tr("Objekt-ID"), tr("Beschreibung")
    });
}

void MainWindow::allesFuellen()
{
    m_tabs->setTabText(0, tr("Anlagen (%1)").arg(m_daten->anlagen().size()));
    m_tabs->setTabText(4, tr("Datenprüfung (%1 Flags)").arg(m_daten->alleProbleme().size()));

    // Linien-Filter und Pareto-Filter fuellen
    const QString alteLinie = m_linieCombo->currentData().toString();
    const QString altePareto = m_paretoLinieCombo->currentData().toString();

    m_linieCombo->blockSignals(true);
    m_linieCombo->clear();
    m_linieCombo->addItem(tr("Alle Linien"), "");
    for (const Linie& l : m_daten->linien()) {
        m_linieCombo->addItem(QString("%1 - %2").arg(l.linieId, l.bezeichnung), l.linieId);
    }
    // Auch fehlerhafte Linien-IDs aus anlagen.csv im Filter anbieten
    for (const Anlage& a : m_daten->anlagen()) {
        if (m_linieCombo->findData(a.linieId) < 0) {
            m_linieCombo->addItem(tr("%1 (unbekannt)").arg(a.linieId), a.linieId);
        }
    }
    const int lIdx = m_linieCombo->findData(alteLinie);
    if (lIdx >= 0) m_linieCombo->setCurrentIndex(lIdx);
    m_linieCombo->blockSignals(false);

    m_paretoLinieCombo->blockSignals(true);
    m_paretoLinieCombo->clear();
    m_paretoLinieCombo->addItem(tr("Alle Linien (Gesamtwerk)"), "");
    for (const Linie& l : m_daten->linien()) {
        m_paretoLinieCombo->addItem(QString("%1 - %2").arg(l.linieId, l.bezeichnung), l.linieId);
    }
    const int pIdx = m_paretoLinieCombo->findData(altePareto);
    if (pIdx >= 0) m_paretoLinieCombo->setCurrentIndex(pIdx);
    m_paretoLinieCombo->blockSignals(false);

    // Dateifilter der Flag-Tabelle fuellen
    const QString alteFlagDatei = m_flagDateiCombo->currentData().toString();
    m_flagDateiCombo->blockSignals(true);
    m_flagDateiCombo->clear();
    m_flagDateiCombo->addItem(tr("Alle Dateien"), "");
    QStringList dateien;
    for (const Problem& p : m_daten->alleProbleme()) {
        if (!dateien.contains(p.datei)) dateien.append(p.datei);
    }
    std::sort(dateien.begin(), dateien.end());
    for (const QString& d : dateien) {
        m_flagDateiCombo->addItem(d, d);
    }
    const int fdIdx = m_flagDateiCombo->findData(alteFlagDatei);
    if (fdIdx >= 0) m_flagDateiCombo->setCurrentIndex(fdIdx);
    m_flagDateiCombo->blockSignals(false);

    m_anlagenModell->aktualisieren();
    m_linienAnsicht->aktualisieren();
    slotParetoAktualisieren();
    stammdatenTabsFuellen();
    flagTabelleFuellen();
    flagLeisteAktualisieren();
    slotFilterGeaendert();
    statusLeisteAktualisieren();
}

void MainWindow::flagLeisteAktualisieren()
{
    const int fehler = m_daten->anzahlProbleme(Schweregrad::Fehler);
    const int warnungen = m_daten->anzahlProbleme(Schweregrad::Warnung);
    if (fehler == 0 && warnungen == 0) {
        m_flagLeiste->hide();
        return;
    }

    m_flagLeisteText->setProperty("meldung", fehler > 0 ? "fehler" : "warn");
    m_flagLeisteText->style()->unpolish(m_flagLeisteText);
    m_flagLeisteText->style()->polish(m_flagLeisteText);
    m_flagLeisteText->setText(
        tr("Automatische Datenprüfung (%1 Protokolle geprüft): %2 Fehler und %3 Warnungen erkannt und mit Flags markiert.")
            .arg(m_daten->anzahlProtokollTagePruefung())
            .arg(fehler)
            .arg(warnungen)
    );
    m_flagLeiste->show();
}

void MainWindow::statusLeisteAktualisieren()
{
    m_statusOrdner->setText(tr("Daten: %1").arg(m_daten->ordner()));
    m_statusBenutzer->setText(tr("Angemeldet: %1 (%2)")
                                  .arg(m_benutzer->aktuellerName(), m_benutzer->rollenName()));
    m_statusFlags->setText(m_daten->pruefungLaeuft()
                               ? tr("Protokollprüfung läuft...")
                               : tr("Flags: %1 Fehler, %2 Warnungen")
                                     .arg(m_daten->anzahlProbleme(Schweregrad::Fehler))
                                     .arg(m_daten->anzahlProbleme(Schweregrad::Warnung)));
    m_statusCache->setText(tr("Cache: %1/%2 (Treffer %3)")
                               .arg(m_daten->cacheGroesse())
                               .arg(Einstellungen::cacheGroesse())
                               .arg(m_daten->cacheTreffer()));
}

static void tabelleSpaltenSicherAnpassen(QTableView* view)
{
    if (!view || !view->model()) return;
    view->resizeColumnsToContents();
    const int spalten = view->model()->columnCount();
    for (int c = 0; c < spalten; c++) {
        const int kopfBreite = view->horizontalHeader()->sectionSizeHint(c);
        const int breit = std::max(view->columnWidth(c), kopfBreite);
        view->setColumnWidth(c, std::clamp(breit + 16, 60, 420));
    }
}

void MainWindow::stammdatenTabsFuellen()
{
    // Linien
    const QList<Linie>& linien = m_daten->linien();
    m_linienTabelle->setUpdatesEnabled(false);
    m_linienTabelle->setRowCount(linien.size());
    for (int i = 0; i < linien.size(); i++) {
        const Linie& l = linien[i];
        int anlagenAnzahl = 0;
        double summeOee = 0.0;
        int oeeAnzahl = 0;
        for (const Anlage& a : m_daten->anlagen()) {
            if (a.linieId != l.linieId) continue;
            anlagenAnzahl++;
            const Trendbericht b = m_daten->trendbericht(a.anlageId);
            if (b.gueltigeTage > 0) {
                summeOee += b.mittelOee;
                oeeAnzahl++;
            }
        }
        m_linienTabelle->setItem(i, 0, new QTableWidgetItem(l.linieId));
        m_linienTabelle->setItem(i, 1, new QTableWidgetItem(l.bezeichnung));
        m_linienTabelle->setItem(i, 2, new QTableWidgetItem(l.halle));
        m_linienTabelle->setItem(i, 3, new QTableWidgetItem(l.schichtmodell));
        m_linienTabelle->setItem(i, 4, new QTableWidgetItem(QString::number(anlagenAnzahl)));
        m_linienTabelle->setItem(i, 5, new QTableWidgetItem(
            oeeAnzahl > 0 ? QString("%1 %").arg(summeOee / oeeAnzahl * 100.0, 0, 'f', 1) : "-"));
    }
    m_linienTabelle->setUpdatesEnabled(true);
    tabelleSpaltenSicherAnpassen(m_linienTabelle);

    // Produkte
    const QList<Produkt>& produkte = m_daten->produkte();
    m_produkteTabelle->setUpdatesEnabled(false);
    m_produkteTabelle->setRowCount(produkte.size());
    for (int i = 0; i < produkte.size(); i++) {
        const Produkt& p = produkte[i];
        m_produkteTabelle->setItem(i, 0, new QTableWidgetItem(p.produktId));
        m_produkteTabelle->setItem(i, 1, new QTableWidgetItem(p.linieId));
        m_produkteTabelle->setItem(i, 2, new QTableWidgetItem(p.bezeichnung));

        QTableWidgetItem* flagItem = new QTableWidgetItem(
            p.probleme.isEmpty() ? tr("OK") : p.probleme.first().text());
        if (!p.probleme.isEmpty()) {
            flagItem->setBackground(Farben::fehlerHintergrund());
            flagItem->setForeground(Farben::fehlerText());
        }
        m_produkteTabelle->setItem(i, 3, flagItem);
    }
    m_produkteTabelle->setUpdatesEnabled(true);
    tabelleSpaltenSicherAnpassen(m_produkteTabelle);

    // Wartungen
    const QList<Wartung>& wartungen = m_daten->wartungen();
    m_wartungenTabelle->setUpdatesEnabled(false);
    m_wartungenTabelle->setRowCount(wartungen.size());
    for (int i = 0; i < wartungen.size(); i++) {
        const Wartung& w = wartungen[i];
        m_wartungenTabelle->setItem(i, 0, new QTableWidgetItem(w.wartungId));
        m_wartungenTabelle->setItem(i, 1, new QTableWidgetItem(w.anlageId));
        m_wartungenTabelle->setItem(i, 2, new QTableWidgetItem(w.datum.toString("yyyy-MM-dd")));
        m_wartungenTabelle->setItem(i, 3, new QTableWidgetItem(w.art));
        m_wartungenTabelle->setItem(i, 4, new QTableWidgetItem(QString::number(w.dauerMin)));
        m_wartungenTabelle->setItem(i, 5, new QTableWidgetItem(w.geplant ? tr("Ja") : tr("Nein")));

        QString text = w.bemerkung;
        if (!w.probleme.isEmpty()) {
            text = QString("[%1] %2  |  %3").arg(w.probleme.first().gradText(),
                                                  w.probleme.first().text(), w.bemerkung);
        }
        QTableWidgetItem* bemItem = new QTableWidgetItem(text);
        if (!w.probleme.isEmpty()) {
            bemItem->setBackground(Farben::fehlerHintergrund());
            bemItem->setForeground(Farben::fehlerText());
        }
        m_wartungenTabelle->setItem(i, 6, bemItem);
    }
    m_wartungenTabelle->setUpdatesEnabled(true);
    tabelleSpaltenSicherAnpassen(m_wartungenTabelle);
}

void MainWindow::slotParetoAktualisieren()
{
    const QList<Stoercode>& codes = m_daten->stoercodes();
    m_stoercodeTabelle->setUpdatesEnabled(false);
    m_stoercodeTabelle->setRowCount(codes.size());
    for (int i = 0; i < codes.size(); i++) {
        m_stoercodeTabelle->setItem(i, 0, new QTableWidgetItem(codes[i].code));
        m_stoercodeTabelle->setItem(i, 1, new QTableWidgetItem(codes[i].kategorie));
        m_stoercodeTabelle->setItem(i, 2, new QTableWidgetItem(codes[i].beschreibung));
    }
    m_stoercodeTabelle->setUpdatesEnabled(true);
    tabelleSpaltenSicherAnpassen(m_stoercodeTabelle);

    const QString linieId = m_paretoLinieCombo->currentData().toString();
    const QList<ParetoEintrag> liste = m_daten->pareto(linieId);
    m_paretoTabelle->setUpdatesEnabled(false);
    m_paretoTabelle->setRowCount(liste.size());
    for (int i = 0; i < liste.size(); i++) {
        const ParetoEintrag& e = liste[i];
        const Stoercode* sc = m_daten->findeStoercode(e.code);
        m_paretoTabelle->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_paretoTabelle->setItem(i, 1, new QTableWidgetItem(e.code));
        m_paretoTabelle->setItem(i, 2, new QTableWidgetItem(sc ? sc->kategorie : tr("unbekannt")));
        m_paretoTabelle->setItem(i, 3, new QTableWidgetItem(
            sc ? sc->beschreibung : tr("Nicht in stoercodes.csv hinterlegt")));
        m_paretoTabelle->setItem(i, 4, new QTableWidgetItem(QString::number(e.anzahl)));
        m_paretoTabelle->setItem(i, 5, new QTableWidgetItem(
            QString("%1 min (kum. %2 %)")
                .arg(e.minuten, 0, 'f', 1)
                .arg(e.kumuliertProzent, 0, 'f', 1)));
    }
    m_paretoTabelle->setUpdatesEnabled(true);
    tabelleSpaltenSicherAnpassen(m_paretoTabelle);
}

void MainWindow::flagTabelleFuellen()
{
    const int gesamtFehler = m_daten->anzahlProbleme(Schweregrad::Fehler);
    const int gesamtWarn = m_daten->anzahlProbleme(Schweregrad::Warnung);
    m_flagZusammenfassung->setText(
        tr("Beim Laden des Datensatzes wurden alle CSV-Dateien und %1 XML-Schichtprotokolle automatisch geprüft. "
           "Ergebnis: %2 Fehler und %3 Warnungen. Doppelklick auf eine Zeile öffnet die betroffene Anlage.")
            .arg(m_daten->anzahlProtokollTagePruefung())
            .arg(gesamtFehler)
            .arg(gesamtWarn)
    );
    slotFlagFilterGeaendert();
}

void MainWindow::slotFlagFilterGeaendert()
{
    const int gradFilter = m_flagGradCombo->currentData().toInt();
    const QString dateiFilter = m_flagDateiCombo->currentData().toString();
    const QString suchFilter = m_flagSucheEdit->text().trimmed();

    QList<Problem> treffer;
    const QList<Problem>& alle = m_daten->alleProbleme();
    treffer.reserve(alle.size());
    for (const Problem& p : alle) {
        if (gradFilter >= 0 && int(p.grad) != gradFilter) continue;
        if (!dateiFilter.isEmpty() && p.datei != dateiFilter) continue;
        const QString beschreibung = p.text();
        if (!suchFilter.isEmpty()
            && !p.objektId.contains(suchFilter, Qt::CaseInsensitive)
            && !p.datei.contains(suchFilter, Qt::CaseInsensitive)
            && !beschreibung.contains(suchFilter, Qt::CaseInsensitive)) {
            continue;
        }
        treffer.append(p);
    }

    m_flagTabelle->setUpdatesEnabled(false);
    m_flagTabelle->setRowCount(treffer.size());
    for (int r = 0; r < treffer.size(); r++) {
        const Problem& p = treffer[r];
        QTableWidgetItem* gradItem = new QTableWidgetItem(p.gradText());
        if (p.grad == Schweregrad::Fehler) {
            gradItem->setBackground(Farben::fehlerHintergrund());
            gradItem->setForeground(Farben::fehlerText());
        } else if (p.grad == Schweregrad::Warnung) {
            gradItem->setBackground(Farben::warnHintergrund());
            gradItem->setForeground(Farben::warnText());
        }
        m_flagTabelle->setItem(r, 0, gradItem);
        m_flagTabelle->setItem(r, 1, new QTableWidgetItem(p.datei));
        m_flagTabelle->setItem(r, 2, new QTableWidgetItem(p.zeile > 0 ? QString::number(p.zeile) : "-"));
        m_flagTabelle->setItem(r, 3, new QTableWidgetItem(p.objektId));
        m_flagTabelle->setItem(r, 4, new QTableWidgetItem(p.text()));
    }
    m_flagTabelle->setUpdatesEnabled(true);
    tabelleSpaltenSicherAnpassen(m_flagTabelle);
}

void MainWindow::slotFlagDoppelklick(int zeile, int)
{
    QTableWidgetItem* idItem = m_flagTabelle->item(zeile, 3);
    if (!idItem) return;
    const QStringList teile = idItem->text().split(' ', Qt::SkipEmptyParts);
    if (teile.isEmpty()) return;

    const QString anlageId = teile.first();
    if (!m_daten->findeAnlage(anlageId)) return;

    QDate tag;
    if (teile.size() >= 2) {
        tag = QDate::fromString(teile[1], "yyyy-MM-dd");
    }
    anlageDetailOeffnen(anlageId, tag);
}

QString MainWindow::ausgewaehlteAnlageId() const
{
    const QModelIndex idx = m_anlagenView->currentIndex();
    if (!idx.isValid()) return QString();
    return idx.siblingAtColumn(0).data(AnlagenModell::AnlageIdRolle).toString();
}

void MainWindow::anlageDetailOeffnen(const QString& anlageId, const QDate& datum)
{
    if (anlageId.isEmpty()) return;
    AnlageDetailDialog dlg(anlageId, m_daten, m_benutzer, this, datum);
    dlg.exec();
    statusLeisteAktualisieren();
}

void MainWindow::slotAusgewaehlteAnlageOeffnen()
{
    const QString id = ausgewaehlteAnlageId();
    if (!id.isEmpty()) anlageDetailOeffnen(id);
}

void MainWindow::slotAnlageDoppelklick(const QModelIndex& proxyIndex)
{
    if (!proxyIndex.isValid()) return;
    anlageDetailOeffnen(proxyIndex.siblingAtColumn(0).data(AnlagenModell::AnlageIdRolle).toString());
}

void MainWindow::slotAnlageKontextmenue(const QPoint& pos)
{
    const QModelIndex idx = m_anlagenView->indexAt(pos);
    if (!idx.isValid()) return;
    const QString id = idx.siblingAtColumn(0).data(AnlagenModell::AnlageIdRolle).toString();
    const Anlage* a = m_daten->findeAnlage(id);
    if (!a) return;

    QMenu menue(this);
    QAction* actDetail = menue.addAction(tr("Details und Protokolle öffnen (%1)").arg(id));
    menue.addSeparator();
    QAction* actBearbeiten = menue.addAction(tr("Stammdaten bearbeiten..."));
    actBearbeiten->setEnabled(m_benutzer->darfStatusAendern());
    QAction* actWartung = menue.addAction(tr("Neue Wartung erfassen..."));
    actWartung->setEnabled(m_benutzer->darfWartungErfassen());

    QMenu* statusUntermenue = menue.addMenu(tr("Status ändern"));
    statusUntermenue->setEnabled(m_benutzer->darfStatusAendern());
    QAction* stBetrieb = statusUntermenue->addAction(anlagenStatusName(AnlagenStatus::InBetrieb));
    stBetrieb->setData(int(AnlagenStatus::InBetrieb));
    QAction* stStoer = statusUntermenue->addAction(anlagenStatusName(AnlagenStatus::Stoerung));
    stStoer->setData(int(AnlagenStatus::Stoerung));
    QAction* stWart = statusUntermenue->addAction(anlagenStatusName(AnlagenStatus::Wartung));
    stWart->setData(int(AnlagenStatus::Wartung));
    QAction* stUmbau = statusUntermenue->addAction(anlagenStatusName(AnlagenStatus::Umbau));
    stUmbau->setData(int(AnlagenStatus::Umbau));

    QAction* wahl = menue.exec(m_anlagenView->viewport()->mapToGlobal(pos));
    if (!wahl) return;

    if (wahl == actDetail) {
        anlageDetailOeffnen(id);
    } else if (wahl == actBearbeiten) {
        slotAnlageBearbeiten();
    } else if (wahl == actWartung) {
        slotWartungErfassen();
    } else if (wahl->data().isValid()) {
        Anlage kopie = *a;
        kopie.status = static_cast<AnlagenStatus>(wahl->data().toInt());
        kopie.statusRoh = anlagenStatusText(kopie.status);
        m_daten->anlageAendern(kopie);
    }
}

void MainWindow::slotAnlageBearbeiten()
{
    if (!m_benutzer->darfStatusAendern()) return;
    const QString id = ausgewaehlteAnlageId();
    if (id.isEmpty()) return;
    const Anlage* a = m_daten->findeAnlage(id);
    if (!a) return;

    AnlageBearbeitenDialog dlg(*a, m_daten, m_benutzer, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_daten->anlageAendern(dlg.ergebnis());
    }
}

void MainWindow::slotWartungErfassen()
{
    if (!m_benutzer->darfWartungErfassen()) return;
    QString id = ausgewaehlteAnlageId();
    if (id.isEmpty() && !m_daten->anlagen().isEmpty()) {
        id = m_daten->anlagen().first().anlageId;
    }
    WartungDialog dlg(id, m_daten, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_daten->wartungHinzufuegen(dlg.ergebnis());
        QMessageBox::information(this, tr("Wartung gespeichert"),
                                 tr("Die Wartung wurde im Speicher hinzugefügt."));
    }
}

void MainWindow::slotFilterGeaendert()
{
    m_filterModell->setSuchtext(m_sucheEdit->text());
    m_filterModell->setLinieFilter(m_linieCombo->currentData().toString());
    m_filterModell->setTypFilter(m_typCombo->currentData().toInt());
    m_filterModell->setStatusFilter(m_statusCombo->currentData().toInt());
    m_filterModell->setMaxBaujahr(m_baujahrSpin->value() > 2000 ? m_baujahrSpin->value() : 0);
    m_filterModell->setNurAuffaellige(m_auffaelligCheck->isChecked());
    m_filterModell->setNurMitFlag(m_flagCheck->isChecked());

    m_anzahlLabel->setText(tr("Angezeigte Anlagen: %1 von %2")
                               .arg(m_filterModell->rowCount())
                               .arg(m_anlagenModell->rowCount()));
}

void MainWindow::slotFilterZuruecksetzen()
{
    m_sucheEdit->clear();
    m_linieCombo->setCurrentIndex(0);
    m_typCombo->setCurrentIndex(0);
    m_statusCombo->setCurrentIndex(0);
    m_baujahrSpin->setValue(2000);
    m_auffaelligCheck->setChecked(false);
    m_flagCheck->setChecked(false);
    m_filterModell->filterZuruecksetzen();
    slotFilterGeaendert();
}

void MainWindow::slotSpaltenAnpassen()
{
    switch (m_tabs->currentIndex()) {
    case 0:
        tabelleSpaltenSicherAnpassen(m_anlagenView);
        break;
    case 1:
        m_linienAnsicht->spaltenAnpassen();
        break;
    case 2:
        tabelleSpaltenSicherAnpassen(m_stoercodeTabelle);
        tabelleSpaltenSicherAnpassen(m_paretoTabelle);
        break;
    case 3:
        tabelleSpaltenSicherAnpassen(m_linienTabelle);
        tabelleSpaltenSicherAnpassen(m_produkteTabelle);
        tabelleSpaltenSicherAnpassen(m_wartungenTabelle);
        break;
    case 4:
        tabelleSpaltenSicherAnpassen(m_flagTabelle);
        break;
    default:
        tabelleSpaltenSicherAnpassen(m_anlagenView);
        break;
    }
}

void MainWindow::slotFlagsTabZeigen()
{
    m_tabs->setCurrentIndex(4);
}

void MainWindow::slotOrdnerOeffnen()
{
    const QString ordner = QFileDialog::getExistingDirectory(
        this, tr("Datenordner auswählen"), m_daten->ordner());
    if (ordner.isEmpty()) return;

    Einstellungen::setDatenordner(ordner);
    m_daten->datensatzLadenHintergrund(ordner);
}

void MainWindow::slotNeuLaden()
{
    m_daten->datensatzLadenHintergrund(m_daten->ordner());
}

void MainWindow::slotSqliteLaden()
{
    const QString datei = QFileDialog::getOpenFileName(
        this,
        tr("Stammdaten aus SQLite laden"),
        QDir(m_daten->ordner()).filePath("RoboCell_daten.sqlite"),
        "SQLite (*.sqlite *.db)"
    );
    if (datei.isEmpty()) return;

    if (!m_daten->datensatzAusSqliteLaden(datei)) {
        QMessageBox::warning(this, tr("SQLite-Fehler"),
                             tr("Die SQLite-Datenbank konnte nicht geladen werden."));
    }
}

void MainWindow::slotSqliteSpeichern()
{
    if (!m_benutzer->darfSqliteSpeichern()) return;
    const QString datei = QFileDialog::getSaveFileName(
        this,
        tr("Stammdaten in SQLite speichern"),
        QDir(m_daten->ordner()).filePath("RoboCell_daten.sqlite"),
        "SQLite (*.sqlite *.db)"
    );
    if (datei.isEmpty()) return;

    QString fehler;
    if (m_daten->inSqliteSpeichern(datei, &fehler)) {
        QMessageBox::information(this, tr("SQLite gespeichert"),
                                 tr("Stammdaten wurden gespeichert:\n%1").arg(datei));
    } else {
        QMessageBox::critical(this, tr("SQLite-Fehler"), fehler);
    }
}

void MainWindow::slotCsvExportieren()
{
    if (!m_benutzer->darfCsvExportieren()) return;
    QString fehler;
    if (m_daten->csvExportieren(m_daten->ordner(), &fehler)) {
        QMessageBox::information(this, tr("CSV-Export abgeschlossen"),
            tr("Die bearbeiteten Tabellen wurden exportiert nach:\n- %1/anlagen_out.csv\n- %1/wartungen_out.csv")
                .arg(m_daten->ordner()));
    } else {
        QMessageBox::critical(this, tr("Export-Fehler"), fehler);
    }
}

void MainWindow::slotEinstellungenOeffnen()
{
    const QString alterOrdner = Einstellungen::datenordner();
    const bool warDunkel = Farben::dunkelmodus();
    EinstellungenDialog dlg(m_daten, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (Farben::dunkelmodus() != warDunkel) {
            m_anlagenModell->farbenAktualisieren();
            m_linienAnsicht->themaAktualisieren();
            stammdatenTabsFuellen();
            slotFlagFilterGeaendert();
            flagLeisteAktualisieren();
        }
        const QString neuerOrdner = Einstellungen::datenordner();
        if (!neuerOrdner.isEmpty() && neuerOrdner != alterOrdner && neuerOrdner != m_daten->ordner()) {
            m_daten->datensatzLadenHintergrund(neuerOrdner);
        }
        statusLeisteAktualisieren();
    }
}

void MainWindow::slotSpracheDeutsch()
{
    Einstellungen::setSprache("de");
    Sprache::setzen("de");
}

void MainWindow::slotSpracheEnglisch()
{
    Einstellungen::setSprache("en");
    Sprache::setzen("en");
}

void MainWindow::slotBerechtigungenZeigen()
{
    QMessageBox::information(this, tr("Aktuelle Berechtigungen"),
        tr("Benutzer: %1\nRolle: %2\n\n%3")
            .arg(m_benutzer->aktuellerName(),
                 m_benutzer->rollenName(),
                 m_benutzer->rechteBeschreibung()));
}

void MainWindow::slotBenutzerWechseln()
{
    AnmeldeDialog dlg(m_benutzer, this);
    dlg.exec();
}

void MainWindow::slotAbmelden()
{
    m_benutzer->abmelden();
    hide();
    AnmeldeDialog dlg(m_benutzer);
    if (dlg.exec() == QDialog::Accepted) {
        show();
    } else {
        close();
    }
}

void MainWindow::slotOeeHilfe()
{
    QMessageBox::information(this, tr("OEE-Formeln und Kennzahlen"),
        tr("Planbelegungszeit T_plan: Dauer aller Zustände in geplanten Schichten ohne Pause und Wartung.\n"
           "Laufzeit T_lauf: Dauer aller Zustände vom Typ Produktion.\n\n"
           "Verfügbarkeit V = T_lauf / T_plan\n"
           "Leistungsgrad L = t_soll * (n_gut + n_aus) / T_lauf\n"
           "Qualitätsrate Q = n_gut / (n_gut + n_aus)\n"
           "OEE = V * L * Q"));
}

void MainWindow::slotUeberProgramm()
{
    QMessageBox::about(this, tr("Über RoboCell"),
        tr("RoboCell - Kennzahlen einer Fertigung mit Roboterzellen\n"
           "Programmentwurf C++ und Qt 6 (DHBW Ravensburg, Campus Friedrichshafen)"));
}

void MainWindow::slotDatenGeaendert()
{
    allesFuellen();
    tabelleSpaltenSicherAnpassen(m_anlagenView);
}

void MainWindow::slotRollenAktualisieren()
{
    const bool darfBearbeiten = m_benutzer->darfStatusAendern();
    const bool darfWartung = m_benutzer->darfWartungErfassen();
    const bool darfCsv = m_benutzer->darfCsvExportieren();
    const bool darfSqlite = m_benutzer->darfSqliteSpeichern();

    m_actBearbeiten->setEnabled(darfBearbeiten);
    m_bearbeitenBtn->setEnabled(darfBearbeiten);
    m_actWartung->setEnabled(darfWartung);
    m_wartungBtn->setEnabled(darfWartung);
    m_neueWartungTabBtn->setEnabled(darfWartung);
    m_actCsvExport->setEnabled(darfCsv);
    m_actSqliteSpeichern->setEnabled(darfSqlite);

    m_actKontoInfo->setText(tr("Angemeldet: %1 (%2)")
                                .arg(m_benutzer->aktuellerName(), m_benutzer->rollenName()));
    statusLeisteAktualisieren();
}
