#pragma once

#include "model/Entities.h"
#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QCheckBox>

namespace RoboCell {

class DataRepository;

/**
 * @brief Bearbeitungsdialog für Stammdaten einer Anlage (Mindestanforderung 4 & Rollenkonzept).
 */
class AnlageEditDialog : public QDialog {
    Q_OBJECT
public:
    AnlageEditDialog(const Anlage& anlage, DataRepository* repo, QWidget* parent = nullptr);

    Anlage updatedAnlage() const { return m_result; }

private slots:
    void onSaveClicked();

private:
    Anlage m_result;
    DataRepository* m_repo = nullptr;

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

/**
 * @brief Dialog zum Erfassen einer neuen Wartung für eine Anlage (Erweiterung 3 & 4).
 */
class WartungAddDialog : public QDialog {
    Q_OBJECT
public:
    WartungAddDialog(const QString& anlageId, DataRepository* repo, QWidget* parent = nullptr);

    Wartung createdWartung() const { return m_wartung; }

private slots:
    void onSaveClicked();

private:
    Wartung m_wartung;
    DataRepository* m_repo = nullptr;

    QComboBox* m_anlageCombo = nullptr;
    QDateEdit* m_datumEdit = nullptr;
    QComboBox* m_artCombo = nullptr;
    QSpinBox* m_dauerSpin = nullptr;
    QCheckBox* m_geplantCheck = nullptr;
    QLineEdit* m_bemerkungEdit = nullptr;
};

} // namespace RoboCell
