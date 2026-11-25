#include "bearbeitendialoge.h"
#include "fensterhilfe.h"
#include "core/benutzerverwaltung.h"
#include "io/datenverwaltung.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

AnlageBearbeitenDialog::AnlageBearbeitenDialog(const Anlage& anlage, Datenverwaltung* daten,
                                               Benutzerverwaltung* benutzer, QWidget* parent)
    : QDialog(parent)
    , m_ergebnis(anlage)
    , m_daten(daten)
    , m_benutzer(benutzer)
{
    setWindowTitle(tr("Stammdaten bearbeiten - %1").arg(anlage.anlageId));

    const bool vollzugriff = m_benutzer && m_benutzer->darfStammdatenBearbeiten();
    const bool darfStatus = m_benutzer && m_benutzer->darfStatusAendern();

    QVBoxLayout* haupt = new QVBoxLayout(this);
    haupt->setSpacing(10);

    QLabel* rollenHinweis = new QLabel(this);
    rollenHinweis->setWordWrap(true);
    rollenHinweis->setProperty("meldung", "info");
    if (vollzugriff) {
        rollenHinweis->setText(tr("Rolle Administrator: Alle Stammdaten dieser Anlage dürfen geändert werden."));
    } else {
        rollenHinweis->setText(tr("Rolle Instandhaltung: Sie dürfen den Status dieser Anlage ändern. Weitere Stammdaten erfordern Administratorrechte."));
    }
    haupt->addWidget(rollenHinweis);

    // Formular liegt in einer QScrollArea, damit es auf kleinen Bildschirmen scrollt
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget* inhalt = new QWidget(scroll);
    QFormLayout* form = new QFormLayout(inhalt);
    form->setSpacing(8);

    form->addRow(tr("Anlage-ID:"), new QLabel(anlage.anlageId, inhalt));

    m_linieCombo = new QComboBox(inhalt);
    for (const Linie& l : m_daten->linien()) {
        m_linieCombo->addItem(QString("%1 (%2)").arg(l.linieId, l.bezeichnung), l.linieId);
    }
    int lIdx = m_linieCombo->findData(anlage.linieId);
    if (lIdx < 0) {
        m_linieCombo->addItem(tr("%1 (Unbekannte Linie)").arg(anlage.linieId), anlage.linieId);
        lIdx = m_linieCombo->count() - 1;
    }
    m_linieCombo->setCurrentIndex(lIdx);
    m_linieCombo->setEnabled(vollzugriff);
    form->addRow(tr("Fertigungslinie:"), m_linieCombo);

    m_posSpin = new QSpinBox(inhalt);
    m_posSpin->setRange(1, 99);
    m_posSpin->setValue(anlage.position > 0 ? anlage.position : 1);
    m_posSpin->setEnabled(vollzugriff);
    form->addRow(tr("Position im Materialfluss:"), m_posSpin);

    m_bezEdit = new QLineEdit(anlage.bezeichnung, inhalt);
    m_bezEdit->setEnabled(vollzugriff);
    form->addRow(tr("Bezeichnung:"), m_bezEdit);

    m_typCombo = new QComboBox(inhalt);
    const AnlagenTyp typen[8] = {
        AnlagenTyp::CncBeladung, AnlagenTyp::Schweissroboter, AnlagenTyp::Montageroboter,
        AnlagenTyp::Schraubstation, AnlagenTyp::Klebestation, AnlagenTyp::Pruefstation,
        AnlagenTyp::Laserbeschriftung, AnlagenTyp::Palettierer
    };
    for (int i = 0; i < 8; i++) {
        m_typCombo->addItem(QString("%1 (%2)").arg(anlagenTypName(typen[i]), anlagenTypText(typen[i])),
                            int(typen[i]));
    }
    const int tIdx = m_typCombo->findData(int(anlage.typ));
    if (tIdx >= 0) m_typCombo->setCurrentIndex(tIdx);
    m_typCombo->setEnabled(vollzugriff);
    form->addRow(tr("Anlagentyp:"), m_typCombo);

    m_herstellerEdit = new QLineEdit(anlage.hersteller, inhalt);
    m_herstellerEdit->setEnabled(vollzugriff);
    form->addRow(tr("Hersteller:"), m_herstellerEdit);

    m_modellEdit = new QLineEdit(anlage.modell, inhalt);
    m_modellEdit->setEnabled(vollzugriff);
    form->addRow(tr("Modell:"), m_modellEdit);

    m_achsenSpin = new QSpinBox(inhalt);
    m_achsenSpin->setRange(0, 12);
    m_achsenSpin->setValue(anlage.achsen);
    m_achsenSpin->setEnabled(vollzugriff);
    form->addRow(tr("Achsen:"), m_achsenSpin);

    m_traglastSpin = new QDoubleSpinBox(inhalt);
    m_traglastSpin->setRange(0.0, 2000.0);
    m_traglastSpin->setDecimals(1);
    m_traglastSpin->setSuffix(" kg");
    m_traglastSpin->setValue(anlage.traglastKg);
    m_traglastSpin->setEnabled(vollzugriff);
    form->addRow(tr("Traglast:"), m_traglastSpin);

    m_baujahrSpin = new QSpinBox(inhalt);
    m_baujahrSpin->setRange(1980, 2050);
    m_baujahrSpin->setValue(anlage.baujahr);
    m_baujahrSpin->setEnabled(vollzugriff);
    form->addRow(tr("Baujahr:"), m_baujahrSpin);

    m_taktSpin = new QDoubleSpinBox(inhalt);
    m_taktSpin->setRange(0.0, 600.0);
    m_taktSpin->setDecimals(1);
    m_taktSpin->setSingleStep(0.5);
    m_taktSpin->setSuffix(" s");
    m_taktSpin->setValue(anlage.sollTaktzeitS);
    m_taktSpin->setEnabled(vollzugriff);
    form->addRow(tr("Soll-Taktzeit:"), m_taktSpin);

    m_statusCombo = new QComboBox(inhalt);
    const AnlagenStatus stati[4] = {
        AnlagenStatus::InBetrieb, AnlagenStatus::Stoerung,
        AnlagenStatus::Wartung, AnlagenStatus::Umbau
    };
    for (int i = 0; i < 4; i++) {
        m_statusCombo->addItem(QString("%1 (%2)").arg(anlagenStatusName(stati[i]), anlagenStatusText(stati[i])),
                               int(stati[i]));
    }
    const int sIdx = m_statusCombo->findData(int(anlage.status));
    if (sIdx >= 0) m_statusCombo->setCurrentIndex(sIdx);
    m_statusCombo->setEnabled(darfStatus);
    form->addRow(tr("Status:"), m_statusCombo);

    scroll->setWidget(inhalt);
    haupt->addWidget(scroll, 1);

    QHBoxLayout* knopfLeiste = new QHBoxLayout();
    knopfLeiste->addStretch();
    QPushButton* abbrechenBtn = new QPushButton(tr("Abbrechen"), this);
    QPushButton* speichernBtn = new QPushButton(tr("Speichern"), this);
    speichernBtn->setDefault(true);
    speichernBtn->setEnabled(darfStatus);
    knopfLeiste->addWidget(abbrechenBtn);
    knopfLeiste->addWidget(speichernBtn);
    haupt->addLayout(knopfLeiste);

    connect(abbrechenBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(speichernBtn, &QPushButton::clicked, this, &AnlageBearbeitenDialog::slotSpeichern);

    Fensterhilfe::fensterAnBildschirmAnpassen(this, 480, 560, 0.90);
}

void AnlageBearbeitenDialog::slotSpeichern()
{
    if (m_taktSpin->value() <= 0.0) {
        QMessageBox::warning(this, tr("Plausibilitätsprüfung"),
                             tr("Die Soll-Taktzeit muss größer als 0 Sekunden sein."));
        return;
    }

    const int aktuellesJahr = QDate::currentDate().year();
    if (m_baujahrSpin->value() > aktuellesJahr) {
        const QMessageBox::StandardButton antwort = QMessageBox::question(
            this,
            tr("Baujahr prüfen"),
            tr("Das Baujahr %1 liegt in der Zukunft (nach %2). Trotzdem speichern?")
                .arg(m_baujahrSpin->value()).arg(aktuellesJahr)
        );
        if (antwort != QMessageBox::Yes) return;
    }

    m_ergebnis.linieId = m_linieCombo->currentData().toString();
    m_ergebnis.position = m_posSpin->value();
    m_ergebnis.bezeichnung = m_bezEdit->text().trimmed();
    m_ergebnis.typ = static_cast<AnlagenTyp>(m_typCombo->currentData().toInt());
    m_ergebnis.typRoh = anlagenTypText(m_ergebnis.typ);
    m_ergebnis.hersteller = m_herstellerEdit->text().trimmed();
    m_ergebnis.modell = m_modellEdit->text().trimmed();
    m_ergebnis.achsen = m_achsenSpin->value();
    m_ergebnis.traglastKg = m_traglastSpin->value();
    m_ergebnis.baujahr = m_baujahrSpin->value();
    m_ergebnis.sollTaktzeitS = m_taktSpin->value();
    m_ergebnis.status = static_cast<AnlagenStatus>(m_statusCombo->currentData().toInt());
    m_ergebnis.statusRoh = anlagenStatusText(m_ergebnis.status);
    accept();
}

// ===========================================================================
// WartungDialog
// ===========================================================================
WartungDialog::WartungDialog(const QString& anlageId, Datenverwaltung* daten, QWidget* parent)
    : QDialog(parent)
    , m_daten(daten)
{
    setWindowTitle(tr("Neue Wartung erfassen"));

    QVBoxLayout* haupt = new QVBoxLayout(this);
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget* inhalt = new QWidget(scroll);
    QFormLayout* form = new QFormLayout(inhalt);
    form->setSpacing(8);

    m_anlageCombo = new QComboBox(inhalt);
    for (const Anlage& a : m_daten->anlagen()) {
        m_anlageCombo->addItem(QString("%1 - %2").arg(a.anlageId, a.bezeichnung), a.anlageId);
    }
    const int idx = m_anlageCombo->findData(anlageId);
    if (idx >= 0) m_anlageCombo->setCurrentIndex(idx);
    form->addRow(tr("Anlage:"), m_anlageCombo);

    m_datumEdit = new QDateEdit(QDate(2026, 9, 27), inhalt);
    m_datumEdit->setCalendarPopup(true);
    m_datumEdit->setDisplayFormat("yyyy-MM-dd");
    form->addRow(tr("Datum:"), m_datumEdit);

    m_artCombo = new QComboBox(inhalt);
    m_artCombo->addItems({"inspektion", "schmierung", "kalibrierung",
                          "greiferwechsel", "getriebeoel", "reparatur"});
    form->addRow(tr("Wartungsart:"), m_artCombo);

    m_dauerSpin = new QSpinBox(inhalt);
    m_dauerSpin->setRange(5, 1440);
    m_dauerSpin->setSingleStep(15);
    m_dauerSpin->setValue(60);
    m_dauerSpin->setSuffix(" min");
    form->addRow(tr("Dauer:"), m_dauerSpin);

    m_geplantCheck = new QCheckBox(tr("Geplante Wartung"), inhalt);
    m_geplantCheck->setChecked(true);
    form->addRow(tr("Planung:"), m_geplantCheck);

    m_bemerkungEdit = new QLineEdit(tr("Planmäßige Wartung"), inhalt);
    form->addRow(tr("Bemerkung:"), m_bemerkungEdit);

    scroll->setWidget(inhalt);
    haupt->addWidget(scroll, 1);

    QHBoxLayout* knopfLeiste = new QHBoxLayout();
    knopfLeiste->addStretch();
    QPushButton* abbrechenBtn = new QPushButton(tr("Abbrechen"), this);
    QPushButton* speichernBtn = new QPushButton(tr("Wartung speichern"), this);
    speichernBtn->setDefault(true);
    knopfLeiste->addWidget(abbrechenBtn);
    knopfLeiste->addWidget(speichernBtn);
    haupt->addLayout(knopfLeiste);

    connect(abbrechenBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(speichernBtn, &QPushButton::clicked, this, &WartungDialog::slotSpeichern);

    Fensterhilfe::fensterAnBildschirmAnpassen(this, 440, 340, 0.90);
}

void WartungDialog::slotSpeichern()
{
    m_wartung.wartungId = m_daten->naechsteWartungsId();
    m_wartung.anlageId = m_anlageCombo->currentData().toString();
    m_wartung.datum = m_datumEdit->date();
    m_wartung.art = m_artCombo->currentText();
    m_wartung.dauerMin = m_dauerSpin->value();
    m_wartung.geplant = m_geplantCheck->isChecked();
    m_wartung.bemerkung = m_bemerkungEdit->text().trimmed();
    if (m_wartung.bemerkung.isEmpty()) {
        m_wartung.bemerkung = m_wartung.geplant ? tr("Planmäßige Wartung") : tr("Reparatur nach Störung");
    }
    accept();
}
