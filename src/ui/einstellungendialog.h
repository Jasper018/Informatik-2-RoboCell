#ifndef EINSTELLUNGENDIALOG_H
#define EINSTELLUNGENDIALOG_H

#include <QDialog>

class QComboBox;
class QEvent;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class Datenverwaltung;

// Einstellungsdialog (Sprache, Schriftgroesse, Standard-Datenordner, Cache)
class EinstellungenDialog : public QDialog {
    Q_OBJECT

public:
    explicit EinstellungenDialog(Datenverwaltung* daten, QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* ereignis) override;

private slots:
    void slotOrdnerWaehlen();
    void slotStandardOrdner();
    void slotCacheLeeren();
    void slotUebernehmen();

private:
    void texteAktualisieren();
    void cacheAnzeigeAktualisieren();

    Datenverwaltung* m_daten = nullptr;

    QGroupBox* m_darstellungGruppe = nullptr;
    QGroupBox* m_datenGruppe = nullptr;
    QLabel* m_spracheLabel = nullptr;
    QComboBox* m_spracheCombo = nullptr;
    QLabel* m_schriftLabel = nullptr;
    QSpinBox* m_schriftSpin = nullptr;
    QLabel* m_ordnerLabel = nullptr;
    QLineEdit* m_ordnerEdit = nullptr;
    QPushButton* m_ordnerBtn = nullptr;
    QPushButton* m_standardOrdnerBtn = nullptr;
    QLabel* m_cacheLabel = nullptr;
    QSpinBox* m_cacheSpin = nullptr;
    QLabel* m_cacheInfoLabel = nullptr;
    QPushButton* m_cacheLeerenBtn = nullptr;
    QLabel* m_kontrastHinweis = nullptr;
    QPushButton* m_abbrechenBtn = nullptr;
    QPushButton* m_okBtn = nullptr;
};

#endif // EINSTELLUNGENDIALOG_H
