#include "ui/EditDialogs.h"
#include "io/DataRepository.h"
#include "core/AuthManager.h"
#include "core/LanguageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

namespace RoboCell {

AnlageEditDialog::AnlageEditDialog(const Anlage& anlage, DataRepository* repo, QWidget* parent)
    : QDialog(parent), m_result(anlage), m_repo(repo)
{
    setWindowTitle(trBi("Stammdaten bearbeiten – %1", "Edit Master Data – %1").arg(anlage.anlageId));
    setMinimumWidth(450);

    const bool fullEdit = AuthManager::instance().canEditMasterData();
    const bool statusEdit = AuthManager::instance().canChangeStatus();

    auto* mainLayout = new QVBoxLayout(this);

    auto* roleNote = new QLabel(this);
    roleNote->setWordWrap(true);
    roleNote->setStyleSheet("background-color: #eff6ff; border: 1px solid #bfdbfe; border-radius: 5px; padding: 6px; color: #1e3a8a;");
    if (fullEdit) {
        roleNote->setText(trBi(
            "<b>Rolle Admin:</b> Sie können alle Stammdatenfelder bearbeiten (inkl. Korrektur von Datenfehlern).",
            "<b>Role Admin:</b> You can edit all master data fields (including fixing data issues)."
        ));
    } else {
        roleNote->setText(trBi(
            "<b>Rolle Instandhaltung:</b> Sie können den Betriebsstatus der Anlage ändern. Für weitere Stammdaten ist die Rolle Admin erforderlich.",
            "<b>Role Maintenance:</b> You can change the operating status. Role Admin is required for other master data fields."
        ));
    }
    mainLayout->addWidget(roleNote);

    auto* form = new QFormLayout();
    form->setSpacing(8);

    auto* idLabel = new QLabel(QString("<b>%1</b>").arg(anlage.anlageId), this);
    form->addRow(trBi("Anlage-ID:", "Machine ID:"), idLabel);

    m_linieCombo = new QComboBox(this);
    for (const auto& l : m_repo->linien()) {
        m_linieCombo->addItem(QString("%1 (%2)").arg(l.linieId, l.bezeichnung), l.linieId);
    }
    int lIdx = m_linieCombo->findData(anlage.linieId);
    if (lIdx < 0) {
        m_linieCombo->addItem(trBi("%1 (Unbekannte Linie!)", "%1 (Unknown Line!)").arg(anlage.linieId), anlage.linieId);
        lIdx = m_linieCombo->count() - 1;
    }
    m_linieCombo->setCurrentIndex(lIdx);
    m_linieCombo->setEnabled(fullEdit);
    form->addRow(trBi("Fertigungslinie:", "Production Line:"), m_linieCombo);

    m_posSpin = new QSpinBox(this);
    m_posSpin->setRange(1, 99);
    m_posSpin->setValue(anlage.position);
    m_posSpin->setEnabled(fullEdit);
    form->addRow(trBi("Position im Materialfluss:", "Position in Flow:"), m_posSpin);

    m_bezEdit = new QLineEdit(anlage.bezeichnung, this);
    m_bezEdit->setEnabled(fullEdit);
    form->addRow(trBi("Bezeichnung:", "Description:"), m_bezEdit);

    m_typCombo = new QComboBox(this);
    const QList<QPair<QString, AnlagenTyp>> typen = {
        {"cnc_beladung", AnlagenTyp::CncBeladung},
        {"schweissroboter", AnlagenTyp::Schweissroboter},
        {"montageroboter", AnlagenTyp::Montageroboter},
        {"schraubstation", AnlagenTyp::Schraubstation},
        {"klebestation", AnlagenTyp::Klebestation},
        {"pruefstation", AnlagenTyp::Pruefstation},
        {"laserbeschriftung", AnlagenTyp::Laserbeschriftung},
        {"palettierer", AnlagenTyp::Palettierer}
    };
    for (const auto& pair : typen) {
        m_typCombo->addItem(pair.first, static_cast<int>(pair.second));
    }
    int tIdx = m_typCombo->findData(static_cast<int>(anlage.typ));
    if (tIdx >= 0) m_typCombo->setCurrentIndex(tIdx);
    m_typCombo->setEnabled(fullEdit);
    form->addRow(trBi("Anlagentyp:", "Machine Type:"), m_typCombo);

    m_herstellerEdit = new QLineEdit(anlage.hersteller, this);
    m_herstellerEdit->setEnabled(fullEdit);
    form->addRow(trBi("Hersteller:", "Manufacturer:"), m_herstellerEdit);

    m_modellEdit = new QLineEdit(anlage.modell, this);
    m_modellEdit->setEnabled(fullEdit);
    form->addRow(trBi("Modell:", "Model:"), m_modellEdit);

    m_achsenSpin = new QSpinBox(this);
    m_achsenSpin->setRange(0, 12);
    m_achsenSpin->setValue(anlage.achsen);
    m_achsenSpin->setEnabled(fullEdit);
    form->addRow(trBi("Achsen:", "Axes:"), m_achsenSpin);

    m_traglastSpin = new QDoubleSpinBox(this);
    m_traglastSpin->setRange(0.0, 2000.0);
    m_traglastSpin->setDecimals(1);
    m_traglastSpin->setSuffix(" kg");
    m_traglastSpin->setValue(anlage.traglastKg);
    m_traglastSpin->setEnabled(fullEdit);
    form->addRow(trBi("Traglast:", "Payload:"), m_traglastSpin);

    m_baujahrSpin = new QSpinBox(this);
    m_baujahrSpin->setRange(1980, 2050);
    m_baujahrSpin->setValue(anlage.baujahr);
    m_baujahrSpin->setEnabled(fullEdit);
    form->addRow(trBi("Baujahr:", "Build Year:"), m_baujahrSpin);

    m_taktSpin = new QDoubleSpinBox(this);
    m_taktSpin->setRange(0.0, 600.0);
    m_taktSpin->setDecimals(1);
    m_taktSpin->setSingleStep(0.5);
    m_taktSpin->setSuffix(" s");
    m_taktSpin->setValue(anlage.sollTaktzeitS);
    m_taktSpin->setEnabled(fullEdit);
    form->addRow(trBi("Soll-Taktzeit:", "Target Cycle Time:"), m_taktSpin);

    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem("in_betrieb", static_cast<int>(AnlagenStatus::InBetrieb));
    m_statusCombo->addItem("stoerung", static_cast<int>(AnlagenStatus::Stoerung));
    m_statusCombo->addItem("wartung", static_cast<int>(AnlagenStatus::Wartung));
    m_statusCombo->addItem("umbau", static_cast<int>(AnlagenStatus::Umbau));
    int sIdx = m_statusCombo->findData(static_cast<int>(anlage.status));
    if (sIdx >= 0) m_statusCombo->setCurrentIndex(sIdx);
    m_statusCombo->setEnabled(statusEdit);
    form->addRow(trBi("Status:", "Status:"), m_statusCombo);

    mainLayout->addLayout(form);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(trBi("Abbrechen", "Cancel"), this);
    auto* saveBtn = new QPushButton(trBi("Übernehmen & Speichern", "Apply & Save"), this);
    saveBtn->setDefault(true);
    saveBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold; padding: 6px 14px; border-radius: 5px;");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    mainLayout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &AnlageEditDialog::onSaveClicked);
}

void AnlageEditDialog::onSaveClicked() {
    if (m_taktSpin->value() <= 0.0) {
        QMessageBox::warning(this,
            trBi("Plausibilitätsprüfung", "Plausibility Check"),
            trBi("Die Soll-Taktzeit muss größer als 0 Sekunden sein!",
                 "Target cycle time must be greater than 0 seconds!"));
        return;
    }

    if (m_baujahrSpin->value() > 2026) {
        const auto res = QMessageBox::question(this,
            trBi("Plausibilitätswarnung Baujahr", "Build Year Warning"),
            trBi("Das eingegebene Baujahr (%1) liegt in der Zukunft (> 2026). Trotzdem übernehmen?",
                 "The entered build year (%1) is in the future (> 2026). Save anyway?").arg(m_baujahrSpin->value()));
        if (res != QMessageBox::Yes) return;
    }

    m_result.linieId = m_linieCombo->currentData().toString();
    m_result.position = m_posSpin->value();
    m_result.bezeichnung = m_bezEdit->text().trimmed();
    m_result.typ = static_cast<AnlagenTyp>(m_typCombo->currentData().toInt());
    m_result.rawTyp = anlagenTypToCsv(m_result.typ);
    m_result.hersteller = m_herstellerEdit->text().trimmed();
    m_result.modell = m_modellEdit->text().trimmed();
    m_result.achsen = m_achsenSpin->value();
    m_result.traglastKg = m_traglastSpin->value();
    m_result.baujahr = m_baujahrSpin->value();
    m_result.sollTaktzeitS = m_taktSpin->value();
    m_result.status = static_cast<AnlagenStatus>(m_statusCombo->currentData().toInt());
    m_result.rawStatus = anlagenStatusToCsv(m_result.status);

    accept();
}

// ============================================================================
// WartungAddDialog
// ============================================================================

WartungAddDialog::WartungAddDialog(const QString& anlageId, DataRepository* repo, QWidget* parent)
    : QDialog(parent), m_repo(repo)
{
    setWindowTitle(trBi("Neue Wartung erfassen", "Record New Maintenance"));
    setMinimumWidth(420);

    auto* mainLayout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    form->setSpacing(8);

    m_anlageCombo = new QComboBox(this);
    for (const auto& a : m_repo->anlagen()) {
        m_anlageCombo->addItem(QString("%1 – %2").arg(a.anlageId, a.bezeichnung), a.anlageId);
    }
    int aIdx = m_anlageCombo->findData(anlageId);
    if (aIdx >= 0) m_anlageCombo->setCurrentIndex(aIdx);
    form->addRow(trBi("Anlage:", "Machine:"), m_anlageCombo);

    m_datumEdit = new QDateEdit(QDate(2026, 9, 27), this);
    m_datumEdit->setCalendarPopup(true);
    m_datumEdit->setDisplayFormat("yyyy-MM-dd");
    form->addRow(trBi("Datum:", "Date:"), m_datumEdit);

    m_artCombo = new QComboBox(this);
    m_artCombo->addItems({"inspektion", "schmierung", "kalibrierung", "greiferwechsel", "getriebeoel", "reparatur"});
    form->addRow(trBi("Wartungsart:", "Maintenance Type:"), m_artCombo);

    m_dauerSpin = new QSpinBox(this);
    m_dauerSpin->setRange(5, 1440);
    m_dauerSpin->setSingleStep(15);
    m_dauerSpin->setValue(60);
    m_dauerSpin->setSuffix(" min");
    form->addRow(trBi("Dauer:", "Duration:"), m_dauerSpin);

    m_geplantCheck = new QCheckBox(trBi("Geplante Wartung (ja)", "Planned Maintenance (yes)"), this);
    m_geplantCheck->setChecked(true);
    form->addRow(trBi("Planung:", "Planning:"), m_geplantCheck);

    m_bemerkungEdit = new QLineEdit(trBi("Planmäßige Wartung", "Scheduled maintenance"), this);
    form->addRow(trBi("Bemerkung:", "Comment:"), m_bemerkungEdit);

    mainLayout->addLayout(form);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* cancelBtn = new QPushButton(trBi("Abbrechen", "Cancel"), this);
    auto* saveBtn = new QPushButton(trBi("Wartung speichern", "Save Maintenance"), this);
    saveBtn->setDefault(true);
    saveBtn->setStyleSheet("background-color: #16a34a; color: white; font-weight: bold; padding: 6px 14px; border-radius: 5px;");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    mainLayout->addLayout(btnRow);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn, &QPushButton::clicked, this, &WartungAddDialog::onSaveClicked);
}

void WartungAddDialog::onSaveClicked() {
    const int nextNum = m_repo->wartungen().size() + 1;
    m_wartung.wartungId = QString("W%1").arg(nextNum, 4, 10, QChar('0'));
    m_wartung.anlageId = m_anlageCombo->currentData().toString();
    m_wartung.datum = m_datumEdit->date();
    m_wartung.art = m_artCombo->currentText();
    m_wartung.dauerMin = m_dauerSpin->value();
    m_wartung.geplant = m_geplantCheck->isChecked();
    m_wartung.bemerkung = m_bemerkungEdit->text().trimmed();
    if (m_wartung.bemerkung.isEmpty()) {
        m_wartung.bemerkung = m_wartung.geplant ? "Planmäßige Wartung" : "Reparatur nach Störung";
    }
    accept();
}

} // namespace RoboCell
