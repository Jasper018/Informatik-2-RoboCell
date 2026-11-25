#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDate>
#include <QMainWindow>

class QAction;
class QActionGroup;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QEvent;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QSpinBox;
class QTableView;
class QTableWidget;
class QTabWidget;
class QWidget;
class AnlagenFilterModell;
class AnlagenModell;
class Benutzerverwaltung;
class Datenverwaltung;
class LinienAnsichtWidget;

// Hauptfenster der Anwendung mit fester Menueleiste, automatischem Flag-Hinweis
// beim Laden und fuenf Arbeitsbereichen.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(Datenverwaltung* daten, Benutzerverwaltung* benutzer, QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* ereignis) override;
    void closeEvent(QCloseEvent* ereignis) override;

private slots:
    // Datei-Menue
    void slotOrdnerOeffnen();
    void slotNeuLaden();
    void slotSqliteLaden();
    void slotSqliteSpeichern();
    void slotCsvExportieren();

    // Bearbeiten-Menue und Aktionen
    void slotAusgewaehlteAnlageOeffnen();
    void slotAnlageBearbeiten();
    void slotWartungErfassen();
    void slotAnlageDoppelklick(const QModelIndex& proxyIndex);
    void slotAnlageKontextmenue(const QPoint& pos);

    // Ansicht und Filter
    void slotFilterGeaendert();
    void slotFilterZuruecksetzen();
    void slotSpaltenAnpassen();
    void slotFlagsTabZeigen();

    // Einstellungen und Sprache
    void slotEinstellungenOeffnen();
    void slotSpracheDeutsch();
    void slotSpracheEnglisch();

    // Konto
    void slotBerechtigungenZeigen();
    void slotBenutzerWechseln();
    void slotAbmelden();

    // Hilfe
    void slotOeeHilfe();
    void slotUeberProgramm();

    // Sekundaere Ansichten
    void slotParetoAktualisieren();
    void slotFlagFilterGeaendert();
    void slotFlagDoppelklick(int zeile, int spalte);

    // Reaktionen auf Modell- und Benutzerwechsel
    void slotDatenGeaendert();
    void slotRollenAktualisieren();

private:
    void menueLeisteBauen();
    void texteAktualisieren();
    void allesFuellen();
    void flagLeisteAktualisieren();
    void statusLeisteAktualisieren();
    void stammdatenTabsFuellen();
    void flagTabelleFuellen();
    QString ausgewaehlteAnlageId() const;
    void anlageDetailOeffnen(const QString& anlageId, const QDate& datum = QDate());

    Datenverwaltung* m_daten = nullptr;
    Benutzerverwaltung* m_benutzer = nullptr;

    // Feste Menueleiste (File, Edit, View, Settings, Account, Help)
    QMenu* m_dateiMenu = nullptr;
    QMenu* m_bearbeitenMenu = nullptr;
    QMenu* m_ansichtMenu = nullptr;
    QMenu* m_einstellungenMenu = nullptr;
    QMenu* m_spracheUntermenu = nullptr;
    QMenu* m_kontoMenu = nullptr;
    QMenu* m_hilfeMenu = nullptr;

    QAction* m_actOrdner = nullptr;
    QAction* m_actNeuLaden = nullptr;
    QAction* m_actSqliteLaden = nullptr;
    QAction* m_actSqliteSpeichern = nullptr;
    QAction* m_actCsvExport = nullptr;
    QAction* m_actBeenden = nullptr;

    QAction* m_actDetails = nullptr;
    QAction* m_actBearbeiten = nullptr;
    QAction* m_actWartung = nullptr;

    QAction* m_actTabAnlagen = nullptr;
    QAction* m_actTabLinien = nullptr;
    QAction* m_actTabPareto = nullptr;
    QAction* m_actTabStamm = nullptr;
    QAction* m_actTabFlags = nullptr;
    QAction* m_actFilterReset = nullptr;
    QAction* m_actSpaltenAnpassen = nullptr;

    QAction* m_actEinstellungen = nullptr;
    QAction* m_actDeutsch = nullptr;
    QAction* m_actEnglisch = nullptr;
    QActionGroup* m_spracheGruppe = nullptr;

    QAction* m_actKontoInfo = nullptr;
    QAction* m_actRechte = nullptr;
    QAction* m_actBenutzerWechseln = nullptr;
    QAction* m_actAbmelden = nullptr;

    QAction* m_actOeeHilfe = nullptr;
    QAction* m_actUeber = nullptr;

    // Automatische Flag-Leiste unter der Menueleiste
    QWidget* m_flagLeiste = nullptr;
    QLabel* m_flagLeisteText = nullptr;
    QPushButton* m_flagLeisteBtn = nullptr;

    QTabWidget* m_tabs = nullptr;

    // Tab 1: Anlagen
    QGroupBox* m_filterGruppe = nullptr;
    QLabel* m_sucheLabel = nullptr;
    QLineEdit* m_sucheEdit = nullptr;
    QLabel* m_linieLabel = nullptr;
    QComboBox* m_linieCombo = nullptr;
    QLabel* m_typLabel = nullptr;
    QComboBox* m_typCombo = nullptr;
    QLabel* m_statusLabel = nullptr;
    QComboBox* m_statusCombo = nullptr;
    QLabel* m_baujahrLabel = nullptr;
    QSpinBox* m_baujahrSpin = nullptr;
    QCheckBox* m_auffaelligCheck = nullptr;
    QCheckBox* m_flagCheck = nullptr;
    QPushButton* m_filterResetBtn = nullptr;

    AnlagenModell* m_anlagenModell = nullptr;
    AnlagenFilterModell* m_filterModell = nullptr;
    QTableView* m_anlagenView = nullptr;
    QLabel* m_anzahlLabel = nullptr;
    QPushButton* m_detailBtn = nullptr;
    QPushButton* m_bearbeitenBtn = nullptr;
    QPushButton* m_wartungBtn = nullptr;

    // Tab 2: Linienansicht
    LinienAnsichtWidget* m_linienAnsicht = nullptr;

    // Tab 3: Stoercodes & Pareto
    QLabel* m_paretoFilterLabel = nullptr;
    QComboBox* m_paretoLinieCombo = nullptr;
    QTableWidget* m_stoercodeTabelle = nullptr;
    QTableWidget* m_paretoTabelle = nullptr;

    // Tab 4: Stammdaten (Linien, Produkte, Wartungen)
    QLabel* m_linienUeberschrift = nullptr;
    QLabel* m_produkteUeberschrift = nullptr;
    QLabel* m_wartungenUeberschrift = nullptr;
    QPushButton* m_neueWartungTabBtn = nullptr;
    QTableWidget* m_linienTabelle = nullptr;
    QTableWidget* m_produkteTabelle = nullptr;
    QTableWidget* m_wartungenTabelle = nullptr;

    // Tab 5: Datenpruefung (Flags)
    QLabel* m_flagZusammenfassung = nullptr;
    QLabel* m_flagGradLabel = nullptr;
    QComboBox* m_flagGradCombo = nullptr;
    QLabel* m_flagDateiLabel = nullptr;
    QComboBox* m_flagDateiCombo = nullptr;
    QLabel* m_flagSucheLabel = nullptr;
    QLineEdit* m_flagSucheEdit = nullptr;
    QTableWidget* m_flagTabelle = nullptr;

    // Statusleiste
    QLabel* m_statusOrdner = nullptr;
    QLabel* m_statusBenutzer = nullptr;
    QLabel* m_statusFlags = nullptr;
    QLabel* m_statusCache = nullptr;
};

#endif // MAINWINDOW_H
