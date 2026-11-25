#include "anmeldedialog.h"
#include "fensterhilfe.h"
#include "core/benutzerverwaltung.h"
#include <QCheckBox>
#include <QEvent>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AnmeldeDialog::AnmeldeDialog(Benutzerverwaltung* benutzer, QWidget* parent)
    : QDialog(parent)
    , m_benutzer(benutzer)
{
    setModal(true);

    QVBoxLayout* haupt = new QVBoxLayout(this);
    haupt->setSpacing(10);
    haupt->setContentsMargins(20, 18, 20, 18);

    m_titelLabel = new QLabel(this);
    QFont titelSchrift = font();
    titelSchrift.setPointSize(titelSchrift.pointSize() + 3);
    titelSchrift.setBold(true);
    m_titelLabel->setFont(titelSchrift);
    haupt->addWidget(m_titelLabel);

    m_hinweisLabel = new QLabel(this);
    m_hinweisLabel->setWordWrap(true);
    m_hinweisLabel->setProperty("hinweis", true);
    haupt->addWidget(m_hinweisLabel);

    QFormLayout* formular = new QFormLayout();
    formular->setSpacing(8);

    m_nameLabel = new QLabel(this);
    m_nameEdit = new QLineEdit(this);
    formular->addRow(m_nameLabel, m_nameEdit);

    m_passwortLabel = new QLabel(this);
    m_passwortEdit = new QLineEdit(this);
    m_passwortEdit->setEchoMode(QLineEdit::Password);
    formular->addRow(m_passwortLabel, m_passwortEdit);

    m_zeigenCheck = new QCheckBox(this);
    formular->addRow(QString(), m_zeigenCheck);
    haupt->addLayout(formular);

    m_fehlerLabel = new QLabel(this);
    m_fehlerLabel->setWordWrap(true);
    m_fehlerLabel->setProperty("meldung", "fehler");
    m_fehlerLabel->hide();
    haupt->addWidget(m_fehlerLabel);

    QHBoxLayout* knopfLeiste = new QHBoxLayout();
    knopfLeiste->addStretch();
    m_abbrechenBtn = new QPushButton(this);
    m_anmeldenBtn = new QPushButton(this);
    m_anmeldenBtn->setDefault(true);
    knopfLeiste->addWidget(m_abbrechenBtn);
    knopfLeiste->addWidget(m_anmeldenBtn);
    haupt->addLayout(knopfLeiste);

    connect(m_abbrechenBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_anmeldenBtn, &QPushButton::clicked, this, &AnmeldeDialog::slotAnmelden);
    connect(m_zeigenCheck, &QCheckBox::toggled, this, &AnmeldeDialog::slotPasswortAnzeigen);

    texteAktualisieren();
    Fensterhilfe::fensterAnBildschirmAnpassen(this, 420, 250, 0.85);
    m_nameEdit->setFocus();
}

void AnmeldeDialog::changeEvent(QEvent* ereignis)
{
    if (ereignis->type() == QEvent::LanguageChange) {
        texteAktualisieren();
    }
    QDialog::changeEvent(ereignis);
}

void AnmeldeDialog::texteAktualisieren()
{
    setWindowTitle(tr("RoboCell - Anmeldung"));
    m_titelLabel->setText(tr("RoboCell Anmeldung"));
    m_hinweisLabel->setText(tr("Bitte melden Sie sich mit Ihrem Benutzernamen und Passwort an."));
    m_nameLabel->setText(tr("Benutzername:"));
    m_nameEdit->setPlaceholderText(tr("Benutzername eingeben"));
    m_passwortLabel->setText(tr("Passwort:"));
    m_passwortEdit->setPlaceholderText(tr("Passwort eingeben"));
    m_zeigenCheck->setText(tr("Passwort anzeigen"));
    m_abbrechenBtn->setText(tr("Abbrechen"));
    m_anmeldenBtn->setText(tr("Anmelden"));
}

void AnmeldeDialog::slotPasswortAnzeigen(bool sichtbar)
{
    m_passwortEdit->setEchoMode(sichtbar ? QLineEdit::Normal : QLineEdit::Password);
}

void AnmeldeDialog::slotAnmelden()
{
    if (!m_benutzer) return;

    if (m_benutzer->anmelden(m_nameEdit->text(), m_passwortEdit->text())) {
        m_fehlerLabel->hide();
        accept();
    } else {
        m_fehlerLabel->setText(tr("Anmeldung fehlgeschlagen: Benutzername oder Passwort ist falsch."));
        m_fehlerLabel->show();
        m_passwortEdit->clear();
        m_passwortEdit->setFocus();
    }
}
