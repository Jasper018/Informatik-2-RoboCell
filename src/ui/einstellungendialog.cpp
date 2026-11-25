#include "einstellungendialog.h"
#include "fensterhilfe.h"
#include "core/einstellungen.h"
#include "core/sprache.h"
#include "io/datenverwaltung.h"
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

EinstellungenDialog::EinstellungenDialog(Datenverwaltung* daten, QWidget* parent)
    : QDialog(parent)
    , m_daten(daten)
{
    setModal(true);

    QVBoxLayout* haupt = new QVBoxLayout(this);
    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget* inhalt = new QWidget(scroll);
    QVBoxLayout* inhaltLayout = new QVBoxLayout(inhalt);
    inhaltLayout->setSpacing(10);

    // Gruppe Darstellung
    m_darstellungGruppe = new QGroupBox(inhalt);
    QFormLayout* darForm = new QFormLayout(m_darstellungGruppe);
    m_spracheLabel = new QLabel(m_darstellungGruppe);
    m_spracheCombo = new QComboBox(m_darstellungGruppe);
    m_spracheCombo->addItem("Deutsch", "de");
    m_spracheCombo->addItem("English", "en");
    const int sIdx = m_spracheCombo->findData(Sprache::aktuell());
    if (sIdx >= 0) m_spracheCombo->setCurrentIndex(sIdx);
    darForm->addRow(m_spracheLabel, m_spracheCombo);

    m_schriftLabel = new QLabel(m_darstellungGruppe);
    m_schriftSpin = new QSpinBox(m_darstellungGruppe);
    m_schriftSpin->setRange(8, 16);
    m_schriftSpin->setValue(Einstellungen::schriftgroesse());
    m_schriftSpin->setSuffix(" pt");
    darForm->addRow(m_schriftLabel, m_schriftSpin);
    inhaltLayout->addWidget(m_darstellungGruppe);

    // Gruppe Datenbasis & Cache
    m_datenGruppe = new QGroupBox(inhalt);
    QFormLayout* datForm = new QFormLayout(m_datenGruppe);
    m_ordnerLabel = new QLabel(m_datenGruppe);
    m_ordnerEdit = new QLineEdit(Einstellungen::datenordner(), m_datenGruppe);
    QHBoxLayout* ordnerZeile = new QHBoxLayout();
    m_ordnerBtn = new QPushButton(m_datenGruppe);
    m_standardOrdnerBtn = new QPushButton(m_datenGruppe);
    ordnerZeile->addWidget(m_ordnerEdit, 1);
    ordnerZeile->addWidget(m_ordnerBtn);
    ordnerZeile->addWidget(m_standardOrdnerBtn);
    datForm->addRow(m_ordnerLabel, ordnerZeile);

    m_cacheLabel = new QLabel(m_datenGruppe);
    m_cacheSpin = new QSpinBox(m_datenGruppe);
    m_cacheSpin->setRange(10, 500);
    m_cacheSpin->setSingleStep(10);
    m_cacheSpin->setValue(Einstellungen::cacheGroesse());
    datForm->addRow(m_cacheLabel, m_cacheSpin);

    m_cacheInfoLabel = new QLabel(m_datenGruppe);
    m_cacheLeerenBtn = new QPushButton(m_datenGruppe);
    QHBoxLayout* cacheZeile = new QHBoxLayout();
    cacheZeile->addWidget(m_cacheInfoLabel, 1);
    cacheZeile->addWidget(m_cacheLeerenBtn);
    datForm->addRow(QString(), cacheZeile);
    inhaltLayout->addWidget(m_datenGruppe);

    m_kontrastHinweis = new QLabel(inhalt);
    m_kontrastHinweis->setWordWrap(true);
    m_kontrastHinweis->setProperty("meldung", "info");
    inhaltLayout->addWidget(m_kontrastHinweis);

    inhaltLayout->addStretch();
    scroll->setWidget(inhalt);
    haupt->addWidget(scroll, 1);

    QHBoxLayout* knoepfe = new QHBoxLayout();
    knoepfe->addStretch();
    m_abbrechenBtn = new QPushButton(this);
    m_okBtn = new QPushButton(this);
    m_okBtn->setDefault(true);
    knoepfe->addWidget(m_abbrechenBtn);
    knoepfe->addWidget(m_okBtn);
    haupt->addLayout(knoepfe);

    connect(m_ordnerBtn, &QPushButton::clicked, this, &EinstellungenDialog::slotOrdnerWaehlen);
    connect(m_standardOrdnerBtn, &QPushButton::clicked, this, &EinstellungenDialog::slotStandardOrdner);
    connect(m_cacheLeerenBtn, &QPushButton::clicked, this, &EinstellungenDialog::slotCacheLeeren);
    connect(m_abbrechenBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okBtn, &QPushButton::clicked, this, &EinstellungenDialog::slotUebernehmen);

    texteAktualisieren();
    cacheAnzeigeAktualisieren();
    Fensterhilfe::fensterAnBildschirmAnpassen(this, 560, 420, 0.90);
}

void EinstellungenDialog::changeEvent(QEvent* ereignis)
{
    if (ereignis->type() == QEvent::LanguageChange) {
        texteAktualisieren();
        cacheAnzeigeAktualisieren();
    }
    QDialog::changeEvent(ereignis);
}

void EinstellungenDialog::texteAktualisieren()
{
    setWindowTitle(tr("Einstellungen"));
    m_darstellungGruppe->setTitle(tr("Darstellung"));
    m_datenGruppe->setTitle(tr("Datenbasis und Cache"));
    m_spracheLabel->setText(tr("Sprache:"));
    m_schriftLabel->setText(tr("Schriftgröße:"));
    m_ordnerLabel->setText(tr("Standard-Datenordner:"));
    m_ordnerEdit->setPlaceholderText(tr("Leer = automatisch relativ zum Programm suchen"));
    m_ordnerBtn->setText(tr("Durchsuchen..."));
    m_standardOrdnerBtn->setText(tr("Automatisch"));
    m_cacheLabel->setText(tr("Protokoll-Cache (Anzahl Tage):"));
    m_cacheLeerenBtn->setText(tr("Cache leeren"));
    m_kontrastHinweis->setText(tr("Alle Oberflächen- und Diagrammfarben sind auf ein Kontrastverhältnis von mindestens 8:1 ausgelegt."));
    m_abbrechenBtn->setText(tr("Abbrechen"));
    m_okBtn->setText(tr("Übernehmen"));
}

void EinstellungenDialog::cacheAnzeigeAktualisieren()
{
    if (!m_daten) return;
    m_cacheInfoLabel->setText(tr("Belegt: %1 Einträge | Treffer: %2 | Fehlversuche: %3")
                                  .arg(m_daten->cacheGroesse())
                                  .arg(m_daten->cacheTreffer())
                                  .arg(m_daten->cacheFehlversuche()));
}

void EinstellungenDialog::slotOrdnerWaehlen()
{
    const QString start = m_ordnerEdit->text().isEmpty()
        ? (m_daten ? m_daten->ordner() : QString())
        : m_ordnerEdit->text();
    const QString ordner = QFileDialog::getExistingDirectory(this, tr("Datenordner auswählen"), start);
    if (!ordner.isEmpty()) m_ordnerEdit->setText(ordner);
}

void EinstellungenDialog::slotStandardOrdner()
{
    m_ordnerEdit->clear();
}

void EinstellungenDialog::slotCacheLeeren()
{
    if (m_daten) {
        m_daten->cacheLeeren();
        cacheAnzeigeAktualisieren();
    }
}

void EinstellungenDialog::slotUebernehmen()
{
    const QString neueSprache = m_spracheCombo->currentData().toString();
    const int neueSchrift = m_schriftSpin->value();
    const int neuerCache = m_cacheSpin->value();
    const QString neuerOrdner = m_ordnerEdit->text().trimmed();

    Einstellungen::setSprache(neueSprache);
    Einstellungen::setSchriftgroesse(neueSchrift);
    Einstellungen::setDatenordner(neuerOrdner);
    Einstellungen::setCacheGroesse(neuerCache);

    QFont f = QApplication::font();
    f.setPointSize(neueSchrift);
    QApplication::setFont(f);

    if (m_daten) {
        m_daten->setCacheGroesse(neuerCache);
    }
    if (Sprache::aktuell() != neueSprache) {
        Sprache::setzen(neueSprache);
    }
    accept();
}
