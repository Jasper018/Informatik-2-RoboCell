#ifndef BEARBEITENDIALOGE_H
#define BEARBEITENDIALOGE_H

#include <QDialog>
#include "model/datentypen.h"

class QCheckBox;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class Benutzerverwaltung;
class Datenverwaltung;

// Dialog zum Bearbeiten von Stammdaten einer Anlage (rollenabhaengig)
class AnlageBearbeitenDialog : public QDialog {
    Q_OBJECT

public:
    AnlageBearbeitenDialog(const Anlage& anlage, Datenverwaltung* daten,
                           Benutzerverwaltung* benutzer, QWidget* parent = nullptr);

    Anlage ergebnis() const { return m_ergebnis; }

private slots:
    void slotSpeichern();

private:
    Anlage m_ergebnis;
    Datenverwaltung* m_daten = nullptr;
    Benutzerverwaltung* m_benutzer = nullptr;

    QComboBox* m_linieCombo = nullptr;
    QSpinBox* m_posSpin = nullptr;
    QLineEdit* m_bezEdit = nullptr;
    QComboBox* m_typCombo = nullptr;
    QLineEdit* m_herstellerEdit = nullptr;
    QLineEdit* m_modellEdit = nullptr;
    QSpinBox* m_achsenSpin = nullptr;
    QDoubleSpinBox* m_traglastSpin = nullptr;
    QSpinBox* m_baujahrSpin = nullptr;
    QDoubleSpinBox* m_taktSpin = nullptr;
    QComboBox* m_statusCombo = nullptr;
};

// Dialog zum Erfassen einer neuen Wartung
class WartungDialog : public QDialog {
    Q_OBJECT

public:
    WartungDialog(const QString& anlageId, Datenverwaltung* daten, QWidget* parent = nullptr);

    Wartung ergebnis() const { return m_wartung; }

private slots:
    void slotSpeichern();

private:
    Wartung m_wartung;
    Datenverwaltung* m_daten = nullptr;

    QComboBox* m_anlageCombo = nullptr;
    QDateEdit* m_datumEdit = nullptr;
    QComboBox* m_artCombo = nullptr;
    QSpinBox* m_dauerSpin = nullptr;
    QCheckBox* m_geplantCheck = nullptr;
    QLineEdit* m_bemerkungEdit = nullptr;
};

#endif // BEARBEITENDIALOGE_H
