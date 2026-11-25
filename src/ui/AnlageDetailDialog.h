#ifndef ANLAGEDETAILDIALOG_H
#define ANLAGEDETAILDIALOG_H

#include <QDate>
#include <QDialog>

class QComboBox;
class QEvent;
class QGroupBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTabWidget;
class Benutzerverwaltung;
class Datenverwaltung;
class StueckzahlDiagrammWidget;
class TrendDiagrammWidget;
class ZustandsLeisteWidget;

// Detailansicht einer Anlage mit Stammdaten, Protokollen (Lazy Loading),
// Zustands-Zeitleiste, Stueckzahlverlauf, OEE, 14-Tage-Trend, Wartungen und Flags.
class AnlageDetailDialog : public QDialog {
    Q_OBJECT

public:
    AnlageDetailDialog(const QString& anlageId, Datenverwaltung* daten,
                       Benutzerverwaltung* benutzer, QWidget* parent = nullptr,
                       const QDate& anfangsDatum = QDate());

protected:
    void changeEvent(QEvent* ereignis) override;

private slots:
    void slotTagGewaehlt(int index);
    void slotBearbeiten();
    void slotWartungErfassen();
    void slotBerichtCsv();
    void slotBerichtPdf();

private:
    void texteAktualisieren();
    void stammdatenAnzeigen();
    void tagesAuswahlFuellen(const QDate& zielDatum);
    void tagLaden(const QDate& datum);
    void trendFuellen();
    void wartungenFuellen();
    void flagsFuellen();

    QString m_anlageId;
    Datenverwaltung* m_daten = nullptr;
    Benutzerverwaltung* m_benutzer = nullptr;

    QLabel* m_titelLabel = nullptr;
    QPushButton* m_bearbeitenBtn = nullptr;
    QPushButton* m_wartungBtn = nullptr;
    QPushButton* m_csvBtn = nullptr;
    QPushButton* m_pdfBtn = nullptr;

    QGroupBox* m_stammGruppe = nullptr;
    QLabel* m_stammLabel = nullptr;
    QLabel* m_linieLabel = nullptr;
    QLabel* m_stammFlagBanner = nullptr;

    QLabel* m_tagAuswahlLabel = nullptr;
    QComboBox* m_tagCombo = nullptr;
    QLabel* m_pfadLabel = nullptr;
    QLabel* m_protokollBanner = nullptr;

    QGroupBox* m_oeeGruppe = nullptr;
    QLabel* m_vLabel = nullptr;
    QLabel* m_lLabel = nullptr;
    QLabel* m_qLabel = nullptr;
    QLabel* m_oeeLabel = nullptr;
    QProgressBar* m_vBalken = nullptr;
    QProgressBar* m_lBalken = nullptr;
    QProgressBar* m_qBalken = nullptr;
    QProgressBar* m_oeeBalken = nullptr;
    QLabel* m_zeitenLabel = nullptr;
    QLabel* m_zuverlaessigkeitLabel = nullptr;

    QTabWidget* m_tabs = nullptr;
    ZustandsLeisteWidget* m_zeitleiste = nullptr;
    StueckzahlDiagrammWidget* m_stueckChart = nullptr;
    QTableWidget* m_zustandTabelle = nullptr;
    QTableWidget* m_schichtTabelle = nullptr;
    TrendDiagrammWidget* m_trendChart = nullptr;
    QLabel* m_trendHinweisLabel = nullptr;
    QTableWidget* m_trendTabelle = nullptr;
    QTableWidget* m_wartungTabelle = nullptr;
    QTableWidget* m_flagTabelle = nullptr;
};

#endif // ANLAGEDETAILDIALOG_H
