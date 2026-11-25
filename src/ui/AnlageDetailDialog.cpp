#include "anlagedetaildialog.h"
#include "bearbeitendialoge.h"
#include "diagramme.h"
#include "fensterhilfe.h"
#include "core/benutzerverwaltung.h"
#include "core/farben.h"
#include "io/datenverwaltung.h"
#include "logic/oeeberechnung.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPrinter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextDocument>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

static QProgressBar* neuerKennzahlBalken(const QString& objektName, QWidget* parent)
{
    QProgressBar* b = new QProgressBar(parent);
    b->setObjectName(objektName);
    b->setRange(0, 1000);
    b->setTextVisible(true);
    b->setMinimumHeight(22);
    return b;
}

static QIcon farbKaestchenIcon(const QColor& farbe)
{
    QPixmap pm(14, 14);
    pm.fill(farbe);
    return QIcon(pm);
}

AnlageDetailDialog::AnlageDetailDialog(const QString& anlageId, Datenverwaltung* daten,
                                       Benutzerverwaltung* benutzer, QWidget* parent,
                                       const QDate& anfangsDatum)
    : QDialog(parent)
    , m_anlageId(anlageId)
    , m_daten(daten)
    , m_benutzer(benutzer)
{
    QVBoxLayout* haupt = new QVBoxLayout(this);
    haupt->setSpacing(8);

    // Kopfzeile
    QHBoxLayout* kopf = new QHBoxLayout();
    m_titelLabel = new QLabel(this);
    QFont titelSchrift = font();
    titelSchrift.setPointSize(titelSchrift.pointSize() + 3);
    titelSchrift.setBold(true);
    m_titelLabel->setFont(titelSchrift);

    m_bearbeitenBtn = new QPushButton(this);
    m_bearbeitenBtn->setEnabled(m_benutzer && m_benutzer->darfStatusAendern());

    m_wartungBtn = new QPushButton(this);
    m_wartungBtn->setEnabled(m_benutzer && m_benutzer->darfWartungErfassen());

    m_csvBtn = new QPushButton(this);
    m_pdfBtn = new QPushButton(this);

    kopf->addWidget(m_titelLabel, 1);
    kopf->addWidget(m_bearbeitenBtn);
    kopf->addWidget(m_wartungBtn);
    kopf->addWidget(m_csvBtn);
    kopf->addWidget(m_pdfBtn);
    haupt->addLayout(kopf);

    // Stammdaten und Linienzuordnung
    m_stammGruppe = new QGroupBox(this);
    QGridLayout* stammGrid = new QGridLayout(m_stammGruppe);
    m_stammLabel = new QLabel(m_stammGruppe);
    m_stammLabel->setWordWrap(true);
    m_linieLabel = new QLabel(m_stammGruppe);
    m_linieLabel->setWordWrap(true);
    stammGrid->addWidget(m_stammLabel, 0, 0);
    stammGrid->addWidget(m_linieLabel, 0, 1);
    haupt->addWidget(m_stammGruppe);

    m_stammFlagBanner = new QLabel(this);
    m_stammFlagBanner->setWordWrap(true);
    m_stammFlagBanner->hide();
    haupt->addWidget(m_stammFlagBanner);

    // Tagesauswahl (Lazy Loading)
    QHBoxLayout* tagLeiste = new QHBoxLayout();
    m_tagAuswahlLabel = new QLabel(this);
    QFont fett = font();
    fett.setBold(true);
    m_tagAuswahlLabel->setFont(fett);

    m_tagCombo = new QComboBox(this);
    m_tagCombo->setMinimumWidth(190);
    m_pfadLabel = new QLabel(this);
    m_pfadLabel->setProperty("hinweis", true);

    tagLeiste->addWidget(m_tagAuswahlLabel);
    tagLeiste->addWidget(m_tagCombo);
    tagLeiste->addSpacing(10);
    tagLeiste->addWidget(m_pfadLabel, 1);
    haupt->addLayout(tagLeiste);

    m_protokollBanner = new QLabel(this);
    m_protokollBanner->setWordWrap(true);
    m_protokollBanner->hide();
    haupt->addWidget(m_protokollBanner);

    // OEE-Kennzahlen
    m_oeeGruppe = new QGroupBox(this);
    QGridLayout* oeeGrid = new QGridLayout(m_oeeGruppe);
    m_vLabel = new QLabel(m_oeeGruppe);
    m_lLabel = new QLabel(m_oeeGruppe);
    m_qLabel = new QLabel(m_oeeGruppe);
    m_oeeLabel = new QLabel(m_oeeGruppe);

    m_vBalken = neuerKennzahlBalken("pb_v", m_oeeGruppe);
    m_lBalken = neuerKennzahlBalken("pb_l", m_oeeGruppe);
    m_qBalken = neuerKennzahlBalken("pb_q", m_oeeGruppe);
    m_oeeBalken = neuerKennzahlBalken("pb_oee", m_oeeGruppe);

    oeeGrid->addWidget(m_vLabel, 0, 0);
    oeeGrid->addWidget(m_vBalken, 0, 1);
    oeeGrid->addWidget(m_lLabel, 0, 2);
    oeeGrid->addWidget(m_lBalken, 0, 3);
    oeeGrid->addWidget(m_qLabel, 1, 0);
    oeeGrid->addWidget(m_qBalken, 1, 1);
    oeeGrid->addWidget(m_oeeLabel, 1, 2);
    oeeGrid->addWidget(m_oeeBalken, 1, 3);

    m_zeitenLabel = new QLabel(m_oeeGruppe);
    m_zeitenLabel->setWordWrap(true);
    m_zuverlaessigkeitLabel = new QLabel(m_oeeGruppe);
    m_zuverlaessigkeitLabel->setWordWrap(true);
    oeeGrid->addWidget(m_zeitenLabel, 2, 0, 1, 2);
    oeeGrid->addWidget(m_zuverlaessigkeitLabel, 2, 2, 1, 2);
    haupt->addWidget(m_oeeGruppe);

    // Registerkarten
    m_tabs = new QTabWidget(this);

    // Tab 1: Diagramme innerhalb QScrollArea, damit auch kleine Monitore alles erreichen
    QScrollArea* verlaufScroll = new QScrollArea(m_tabs);
    verlaufScroll->setWidgetResizable(true);
    verlaufScroll->setFrameShape(QFrame::NoFrame);
    QWidget* verlaufInhalt = new QWidget(verlaufScroll);
    QVBoxLayout* verlaufLayout = new QVBoxLayout(verlaufInhalt);
    m_zeitleiste = new ZustandsLeisteWidget(verlaufInhalt);
    m_stueckChart = new StueckzahlDiagrammWidget(verlaufInhalt);
    verlaufLayout->addWidget(m_zeitleiste);
    verlaufLayout->addWidget(m_stueckChart);
    verlaufScroll->setWidget(verlaufInhalt);
    m_tabs->addTab(verlaufScroll, QString());

    // Tab 2: Zeitanteile und Schichtvergleich
    QSplitter* tab2Teiler = new QSplitter(Qt::Horizontal, m_tabs);
    tab2Teiler->setChildrenCollapsible(false);

    m_zustandTabelle = new QTableWidget(tab2Teiler);
    m_zustandTabelle->setColumnCount(4);
    m_zustandTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_zustandTabelle->setAlternatingRowColors(true);
    m_zustandTabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_zustandTabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_zustandTabelle->horizontalHeader()->setStretchLastSection(true);
    m_zustandTabelle->verticalHeader()->setVisible(false);

    m_schichtTabelle = new QTableWidget(tab2Teiler);
    m_schichtTabelle->setColumnCount(7);
    m_schichtTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_schichtTabelle->setAlternatingRowColors(true);
    m_schichtTabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_schichtTabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_schichtTabelle->horizontalHeader()->setStretchLastSection(true);
    m_schichtTabelle->verticalHeader()->setVisible(false);

    tab2Teiler->addWidget(m_zustandTabelle);
    tab2Teiler->addWidget(m_schichtTabelle);
    m_tabs->addTab(tab2Teiler, QString());

    // Tab 3: 14-Tage-Trend
    QWidget* tabTrend = new QWidget(m_tabs);
    QVBoxLayout* trendLayout = new QVBoxLayout(tabTrend);
    m_trendHinweisLabel = new QLabel(tabTrend);
    m_trendHinweisLabel->setWordWrap(true);
    trendLayout->addWidget(m_trendHinweisLabel);

    QSplitter* trendTeiler = new QSplitter(Qt::Vertical, tabTrend);
    trendTeiler->setChildrenCollapsible(false);
    m_trendChart = new TrendDiagrammWidget(trendTeiler);
    m_trendTabelle = new QTableWidget(trendTeiler);
    m_trendTabelle->setColumnCount(8);
    m_trendTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trendTabelle->setAlternatingRowColors(true);
    m_trendTabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_trendTabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_trendTabelle->horizontalHeader()->setStretchLastSection(true);
    m_trendTabelle->verticalHeader()->setVisible(false);
    trendTeiler->addWidget(m_trendChart);
    trendTeiler->addWidget(m_trendTabelle);
    trendTeiler->setStretchFactor(0, 3);
    trendTeiler->setStretchFactor(1, 2);
    trendLayout->addWidget(trendTeiler, 1);
    m_tabs->addTab(tabTrend, QString());

    // Tab 4: Wartungshistorie
    QWidget* tabWart = new QWidget(m_tabs);
    QVBoxLayout* wartLayout = new QVBoxLayout(tabWart);
    m_wartungTabelle = new QTableWidget(tabWart);
    m_wartungTabelle->setColumnCount(6);
    m_wartungTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_wartungTabelle->setAlternatingRowColors(true);
    m_wartungTabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_wartungTabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_wartungTabelle->horizontalHeader()->setStretchLastSection(true);
    m_wartungTabelle->verticalHeader()->setVisible(false);
    wartLayout->addWidget(m_wartungTabelle);
    m_tabs->addTab(tabWart, QString());

    // Tab 5: Erkannte Flags dieser Anlage
    QWidget* tabFlags = new QWidget(m_tabs);
    QVBoxLayout* flagLayout = new QVBoxLayout(tabFlags);
    m_flagTabelle = new QTableWidget(tabFlags);
    m_flagTabelle->setColumnCount(4);
    m_flagTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_flagTabelle->setAlternatingRowColors(true);
    m_flagTabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_flagTabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_flagTabelle->horizontalHeader()->setStretchLastSection(true);
    m_flagTabelle->verticalHeader()->setVisible(false);
    flagLayout->addWidget(m_flagTabelle);
    m_tabs->addTab(tabFlags, QString());

    haupt->addWidget(m_tabs, 1);

    connect(m_tagCombo, &QComboBox::currentIndexChanged, this, &AnlageDetailDialog::slotTagGewaehlt);
    connect(m_bearbeitenBtn, &QPushButton::clicked, this, &AnlageDetailDialog::slotBearbeiten);
    connect(m_wartungBtn, &QPushButton::clicked, this, &AnlageDetailDialog::slotWartungErfassen);
    connect(m_csvBtn, &QPushButton::clicked, this, &AnlageDetailDialog::slotBerichtCsv);
    connect(m_pdfBtn, &QPushButton::clicked, this, &AnlageDetailDialog::slotBerichtPdf);

    texteAktualisieren();
    stammdatenAnzeigen();
    tagesAuswahlFuellen(anfangsDatum);
    trendFuellen();
    wartungenFuellen();
    flagsFuellen();

    if (m_tagCombo->count() > 0) {
        tagLaden(m_tagCombo->currentData().toDate());
    } else {
        tagLaden(QDate());
    }

    Fensterhilfe::fensterAnBildschirmAnpassen(this, 1080, 760, 0.90);
}

void AnlageDetailDialog::changeEvent(QEvent* ereignis)
{
    if (ereignis->type() == QEvent::LanguageChange) {
        texteAktualisieren();
        stammdatenAnzeigen();
        tagesAuswahlFuellen(m_tagCombo->currentData().toDate());
        if (m_tagCombo->currentData().toDate().isValid()) {
            tagLaden(m_tagCombo->currentData().toDate());
        }
        trendFuellen();
        wartungenFuellen();
        flagsFuellen();
    }
    QDialog::changeEvent(ereignis);
}

void AnlageDetailDialog::texteAktualisieren()
{
    m_bearbeitenBtn->setText(tr("Stammdaten bearbeiten"));
    m_wartungBtn->setText(tr("Wartung erfassen"));
    m_csvBtn->setText(tr("Bericht (CSV)"));
    m_pdfBtn->setText(tr("Bericht (PDF)"));
    m_stammGruppe->setTitle(tr("Stammdaten und Linienzuordnung"));
    m_tagAuswahlLabel->setText(tr("Protokolltag (Lazy Loading):"));
    m_oeeGruppe->setTitle(tr("Tagesauswertung: OEE, Faktoren und Zuverlässigkeit"));
    m_vLabel->setText(tr("Verfügbarkeit (V):"));
    m_lLabel->setText(tr("Leistungsgrad (L):"));
    m_qLabel->setText(tr("Qualitätsrate (Q):"));
    m_oeeLabel->setText(tr("Gesamt-OEE (V * L * Q):"));

    m_tabs->setTabText(0, tr("Tagesverlauf"));
    m_tabs->setTabText(1, tr("Zeitanteile und Schichten"));
    m_tabs->setTabText(2, tr("14-Tage-Trend"));
    m_tabs->setTabText(3, tr("Wartungen"));
    m_tabs->setTabText(4, tr("Prüfprotokoll (Flags)"));

    m_zustandTabelle->setHorizontalHeaderLabels({
        tr("Zustandstyp"), tr("Dauer [min]"), tr("Dauer [s]"), tr("Anteil")
    });
    m_schichtTabelle->setHorizontalHeaderLabels({
        tr("Schicht"), tr("Geplant"), tr("V [%]"), tr("L [%]"),
        tr("Q [%]"), tr("OEE [%]"), tr("Gut / Ausschuss")
    });
    m_trendTabelle->setHorizontalHeaderLabels({
        tr("Datum"), tr("V [%]"), tr("L [%]"), tr("Q [%]"),
        tr("OEE [%]"), tr("Gutteile"), tr("Ausschuss"), tr("Störungen")
    });
    m_wartungTabelle->setHorizontalHeaderLabels({
        tr("Wartung-ID"), tr("Datum"), tr("Art"), tr("Dauer [min]"), tr("Geplant"), tr("Bemerkung")
    });
    m_flagTabelle->setHorizontalHeaderLabels({
        tr("Schweregrad"), tr("Datei"), tr("Zeile"), tr("Beschreibung")
    });
}

void AnlageDetailDialog::stammdatenAnzeigen()
{
    const Anlage* a = m_daten->findeAnlage(m_anlageId);
    if (!a) return;

    setWindowTitle(tr("Anlagendetails - %1 (%2)").arg(a->anlageId, a->bezeichnung));
    m_titelLabel->setText(QString("%1 - %2").arg(a->anlageId, a->bezeichnung));

    m_stammLabel->setText(tr("Typ: %1  |  Hersteller/Modell: %2 %3\n"
                             "Achsen: %4  |  Traglast: %5 kg  |  Baujahr: %6\n"
                             "Soll-Taktzeit: %7 s  |  Status: %8")
                              .arg(anlagenTypName(a->typ, a->typRoh), a->hersteller, a->modell)
                              .arg(a->achsen).arg(a->traglastKg, 0, 'f', 1).arg(a->baujahr)
                              .arg(a->sollTaktzeitS, 0, 'f', 1)
                              .arg(anlagenStatusName(a->status, a->statusRoh)));

    const Linie* l = m_daten->findeLinie(a->linieId);
    if (l) {
        m_linieLabel->setText(tr("Fertigungslinie: %1 - %2\n"
                                 "Halle: %3  |  Position im Materialfluss: %4\n"
                                 "Schichtmodell: %5")
                                  .arg(l->linieId, l->bezeichnung, l->halle)
                                  .arg(a->position).arg(l->schichtmodell));
    } else {
        m_linieLabel->setText(tr("Fertigungslinie '%1' ist in linien.csv nicht vorhanden.\n"
                                 "Position im Materialfluss: %2")
                                  .arg(a->linieId).arg(a->position));
    }

    if (a->hatProbleme()) {
        QStringList meldungen;
        for (const Problem& p : a->probleme) {
            meldungen.append(QString("[%1] %2").arg(p.gradText(), p.text()));
        }
        const bool hatFehler = a->anzahlProbleme(Schweregrad::Fehler) > 0;
        m_stammFlagBanner->setProperty("meldung", hatFehler ? "fehler" : "warn");
        m_stammFlagBanner->style()->unpolish(m_stammFlagBanner);
        m_stammFlagBanner->style()->polish(m_stammFlagBanner);
        m_stammFlagBanner->setText(tr("Stammdaten-Flags: ") + meldungen.join("  |  "));
        m_stammFlagBanner->show();
    } else {
        m_stammFlagBanner->hide();
    }
}

void AnlageDetailDialog::tagesAuswahlFuellen(const QDate& zielDatum)
{
    m_tagCombo->blockSignals(true);
    m_tagCombo->clear();
    const QList<QDate> tage = m_daten->verfuegbareTage(m_anlageId);
    for (const QDate& d : tage) {
        const QList<Problem> tagFlags = m_daten->tagesProbleme(m_anlageId, d);
        QString beschriftung = d.toString("yyyy-MM-dd (ddd)");
        if (!tagFlags.isEmpty()) {
            bool hatFehler = false;
            for (const Problem& p : tagFlags) {
                if (p.grad == Schweregrad::Fehler) { hatFehler = true; break; }
            }
            beschriftung += hatFehler ? tr(" [Fehler]") : tr(" [Warnung]");
        }
        m_tagCombo->addItem(beschriftung, d);
    }
    if (zielDatum.isValid()) {
        const int idx = m_tagCombo->findData(zielDatum);
        if (idx >= 0) m_tagCombo->setCurrentIndex(idx);
    }
    m_tagCombo->blockSignals(false);
}

void AnlageDetailDialog::slotTagGewaehlt(int index)
{
    if (index < 0) return;
    tagLaden(m_tagCombo->itemData(index).toDate());
}

void AnlageDetailDialog::tagLaden(const QDate& datum)
{
    const Anlage* a = m_daten->findeAnlage(m_anlageId);
    if (!a) return;

    const QString relPfad = m_daten->protokollPfad(m_anlageId, datum);
    m_pfadLabel->setText(relPfad.isEmpty()
        ? tr("Kein Protokoll im Index")
        : tr("Datei: %1  |  Cache-Treffer: %2").arg(relPfad).arg(m_daten->cacheTreffer()));

    std::shared_ptr<Schichtprotokoll> p = m_daten->protokoll(m_anlageId, datum);
    m_zeitleiste->setProtokoll(p);

    auto meldungSetzen = [&](const QString& art, const QString& text) {
        m_protokollBanner->setProperty("meldung", art);
        m_protokollBanner->style()->unpolish(m_protokollBanner);
        m_protokollBanner->style()->polish(m_protokollBanner);
        m_protokollBanner->setText(text);
        m_protokollBanner->show();
    };

    // Degradationsverhalten bei fehlender oder defekter Datei
    if (!p || !p->geladen) {
        QString grund = tr("Unbekannter Lesefehler");
        if (p && !p->probleme.isEmpty()) {
            grund = p->probleme.first().text();
        }
        meldungSetzen("fehler",
            tr("Degradationsmodus: Das Protokoll für %1 konnte nicht geladen werden (%2). "
               "Stammdaten, Wartungen und andere Tage bleiben nutzbar.")
                .arg(datum.toString("dd.MM.yyyy"), grund));

        m_stueckChart->setVerlauf({});
        m_vBalken->setValue(0); m_vBalken->setFormat("N/A");
        m_lBalken->setValue(0); m_lBalken->setFormat("N/A");
        m_qBalken->setValue(0); m_qBalken->setFormat("N/A");
        m_oeeBalken->setValue(0); m_oeeBalken->setFormat("N/A");
        m_zeitenLabel->setText(tr("Keine Tagesauswertung verfügbar."));
        m_zuverlaessigkeitLabel->clear();
        m_zustandTabelle->setRowCount(0);
        m_schichtTabelle->setRowCount(0);
        return;
    }

    const OeeKennzahlen dm = OeeBerechnung::tag(*p, a->sollTaktzeitS);
    m_stueckChart->setVerlauf(dm.verlauf);

    // Flags aus Index (z.B. XML_ANZAHL) + XML-Pruefung zusammenfassen
    const QList<Problem> flags = m_daten->tagesProbleme(m_anlageId, datum);
    if (!flags.isEmpty()) {
        QStringList texte;
        bool hatFehler = false;
        for (const Problem& pr : flags) {
            if (pr.grad == Schweregrad::Fehler) hatFehler = true;
            texte.append(pr.text());
        }
        meldungSetzen(hatFehler ? "fehler" : "warn",
                      tr("Protokoll-Flags (%1): ").arg(flags.size()) + texte.join("  |  "));
    } else if (!dm.hatGeplanteSchicht) {
        meldungSetzen("info", tr("Am %1 waren keine Schichten geplant (geplant=\"nein\").")
                                  .arg(datum.toString("dd.MM.yyyy")));
    } else {
        m_protokollBanner->hide();
    }

    auto balkenSetzen = [](QProgressBar* bar, double wert) {
        bar->setValue(std::clamp(int(std::round(wert * 1000.0)), 0, 1000));
        bar->setFormat(QString("%1 %").arg(wert * 100.0, 0, 'f', 1));
    };

    if (dm.gueltig) {
        balkenSetzen(m_vBalken, dm.verfuegbarkeit);
        balkenSetzen(m_lBalken, dm.leistungsgrad);
        balkenSetzen(m_qBalken, dm.qualitaet);
        balkenSetzen(m_oeeBalken, dm.oee);
    } else {
        m_vBalken->setValue(0); m_vBalken->setFormat("-");
        m_lBalken->setValue(0); m_lBalken->setFormat("-");
        m_qBalken->setValue(0); m_qBalken->setFormat("-");
        m_oeeBalken->setValue(0); m_oeeBalken->setFormat("-");
    }

    m_zeitenLabel->setText(tr("Planbelegungszeit: %1 min  |  Laufzeit: %2 min  |  Gut: %3  |  Ausschuss: %4")
                               .arg(dm.planzeitSek / 60.0, 0, 'f', 1)
                               .arg(dm.laufzeitSek / 60.0, 0, 'f', 1)
                               .arg(dm.gut).arg(dm.ausschuss));
    m_zuverlaessigkeitLabel->setText(tr("Störungen: %1  |  MTBF: %2 min  |  MTTR: %3 min")
                                         .arg(dm.anzahlStoerungen)
                                         .arg(dm.mtbfMin, 0, 'f', 1)
                                         .arg(dm.mttrMin, 0, 'f', 1));

    // Zeitanteile je Zustand
    const ZustandTyp reihenfolge[8] = {
        ZustandTyp::Produktion, ZustandTyp::Ruesten, ZustandTyp::Stoerung,
        ZustandTyp::Materialmangel, ZustandTyp::Rueckstau, ZustandTyp::Wartung,
        ZustandTyp::Pause, ZustandTyp::Unbekannt
    };
    m_zustandTabelle->setRowCount(0);
    for (int i = 0; i < 8; i++) {
        const ZustandTyp zt = reihenfolge[i];
        const qint64 sek = dm.zustandSek.value(zt, 0);
        if (zt == ZustandTyp::Unbekannt && sek == 0) continue;
        const int r = m_zustandTabelle->rowCount();
        m_zustandTabelle->insertRow(r);

        QTableWidgetItem* nameItem = new QTableWidgetItem(farbKaestchenIcon(Farben::zustandFarbe(zt)),
                                                          zustandName(zt));
        m_zustandTabelle->setItem(r, 0, nameItem);
        m_zustandTabelle->setItem(r, 1, new QTableWidgetItem(QString("%1").arg(sek / 60.0, 0, 'f', 1)));
        m_zustandTabelle->setItem(r, 2, new QTableWidgetItem(QString::number(sek)));
        const double anteil = (dm.schichtzeitSek > 0) ? double(sek) / dm.schichtzeitSek * 100.0 : 0.0;
        m_zustandTabelle->setItem(r, 3, new QTableWidgetItem(QString("%1 %").arg(anteil, 0, 'f', 1)));
    }
    m_zustandTabelle->resizeColumnsToContents();

    // Schichtvergleich
    const QList<OeeKennzahlen> schichten = OeeBerechnung::schichten(*p, a->sollTaktzeitS);
    m_schichtTabelle->setRowCount(schichten.size());
    for (int i = 0; i < schichten.size(); i++) {
        const OeeKennzahlen& sm = schichten[i];
        m_schichtTabelle->setItem(i, 0, new QTableWidgetItem(sm.bezeichnung));
        m_schichtTabelle->setItem(i, 1, new QTableWidgetItem(sm.hatGeplanteSchicht ? tr("Ja") : tr("Nein")));
        m_schichtTabelle->setItem(i, 2, new QTableWidgetItem(sm.gueltig ? QString("%1 %").arg(sm.verfuegbarkeit * 100.0, 0, 'f', 1) : "-"));
        m_schichtTabelle->setItem(i, 3, new QTableWidgetItem(sm.gueltig ? QString("%1 %").arg(sm.leistungsgrad * 100.0, 0, 'f', 1) : "-"));
        m_schichtTabelle->setItem(i, 4, new QTableWidgetItem(sm.gueltig ? QString("%1 %").arg(sm.qualitaet * 100.0, 0, 'f', 1) : "-"));
        m_schichtTabelle->setItem(i, 5, new QTableWidgetItem(sm.gueltig ? QString("%1 %").arg(sm.oee * 100.0, 0, 'f', 1) : "-"));
        m_schichtTabelle->setItem(i, 6, new QTableWidgetItem(QString("%1 / %2").arg(sm.gut).arg(sm.ausschuss)));
    }
    m_schichtTabelle->resizeColumnsToContents();
}

void AnlageDetailDialog::trendFuellen()
{
    const QList<OeeKennzahlen> tage = m_daten->alleTageskennzahlen(m_anlageId);
    const Trendbericht bericht = m_daten->trendbericht(m_anlageId);

    m_trendChart->setTage(tage);
    const QStringList hinweise = bericht.hinweise();

    m_trendHinweisLabel->setProperty("meldung", bericht.auffaellig ? "warn" : "ok");
    m_trendHinweisLabel->style()->unpolish(m_trendHinweisLabel);
    m_trendHinweisLabel->style()->polish(m_trendHinweisLabel);

    if (bericht.auffaellig && !hinweise.isEmpty()) {
        m_trendHinweisLabel->setText(
            tr("Auffällig (14 Tage): %1  |  Ø OEE: %2 %  |  MTBF: %3 min  |  MTTR: %4 min")
                .arg(hinweise.join("; "))
                .arg(bericht.mittelOee * 100.0, 0, 'f', 1)
                .arg(bericht.mtbfMin, 0, 'f', 1)
                .arg(bericht.mttrMin, 0, 'f', 1));
    } else {
        m_trendHinweisLabel->setText(
            tr("14-Tage-Trend unauffällig  |  Ø OEE: %1 %  |  MTBF: %2 min  |  MTTR: %3 min")
                .arg(bericht.mittelOee * 100.0, 0, 'f', 1)
                .arg(bericht.mtbfMin, 0, 'f', 1)
                .arg(bericht.mttrMin, 0, 'f', 1));
    }

    m_trendTabelle->setRowCount(tage.size());
    for (int i = 0; i < tage.size(); i++) {
        const OeeKennzahlen& dm = tage[i];
        m_trendTabelle->setItem(i, 0, new QTableWidgetItem(dm.datum.toString("yyyy-MM-dd")));
        m_trendTabelle->setItem(i, 1, new QTableWidgetItem(dm.gueltig ? QString("%1 %").arg(dm.verfuegbarkeit * 100.0, 0, 'f', 1) : "-"));
        m_trendTabelle->setItem(i, 2, new QTableWidgetItem(dm.gueltig ? QString("%1 %").arg(dm.leistungsgrad * 100.0, 0, 'f', 1) : "-"));
        m_trendTabelle->setItem(i, 3, new QTableWidgetItem(dm.gueltig ? QString("%1 %").arg(dm.qualitaet * 100.0, 0, 'f', 1) : "-"));
        m_trendTabelle->setItem(i, 4, new QTableWidgetItem(dm.gueltig ? QString("%1 %").arg(dm.oee * 100.0, 0, 'f', 1) : "-"));
        m_trendTabelle->setItem(i, 5, new QTableWidgetItem(QString::number(dm.gut)));
        m_trendTabelle->setItem(i, 6, new QTableWidgetItem(QString::number(dm.ausschuss)));
        m_trendTabelle->setItem(i, 7, new QTableWidgetItem(QString("%1 (%2 min)")
            .arg(dm.anzahlStoerungen).arg(dm.stoerzeitSek / 60.0, 0, 'f', 1)));
    }
    m_trendTabelle->resizeColumnsToContents();
}

void AnlageDetailDialog::wartungenFuellen()
{
    const QList<Wartung> liste = m_daten->wartungenVonAnlage(m_anlageId);
    m_wartungTabelle->setRowCount(liste.size());
    for (int i = 0; i < liste.size(); i++) {
        const Wartung& w = liste[i];
        m_wartungTabelle->setItem(i, 0, new QTableWidgetItem(w.wartungId));
        m_wartungTabelle->setItem(i, 1, new QTableWidgetItem(w.datum.toString("yyyy-MM-dd")));
        m_wartungTabelle->setItem(i, 2, new QTableWidgetItem(w.art));
        m_wartungTabelle->setItem(i, 3, new QTableWidgetItem(QString::number(w.dauerMin)));
        m_wartungTabelle->setItem(i, 4, new QTableWidgetItem(w.geplant ? tr("Ja") : tr("Nein")));
        m_wartungTabelle->setItem(i, 5, new QTableWidgetItem(w.bemerkung));
    }
    m_wartungTabelle->resizeColumnsToContents();
}

void AnlageDetailDialog::flagsFuellen()
{
    const QList<Problem> liste = m_daten->anlagenProbleme(m_anlageId);
    m_flagTabelle->setRowCount(liste.size());
    for (int i = 0; i < liste.size(); i++) {
        const Problem& p = liste[i];
        QTableWidgetItem* gradItem = new QTableWidgetItem(p.gradText());
        if (p.grad == Schweregrad::Fehler) {
            gradItem->setBackground(Farben::FEHLER_HINTERGRUND);
            gradItem->setForeground(Farben::FEHLER_TEXT);
        } else if (p.grad == Schweregrad::Warnung) {
            gradItem->setBackground(Farben::WARN_HINTERGRUND);
            gradItem->setForeground(Farben::WARN_TEXT);
        }
        m_flagTabelle->setItem(i, 0, gradItem);
        m_flagTabelle->setItem(i, 1, new QTableWidgetItem(p.datei));
        m_flagTabelle->setItem(i, 2, new QTableWidgetItem(p.zeile > 0 ? QString::number(p.zeile) : "-"));
        m_flagTabelle->setItem(i, 3, new QTableWidgetItem(p.text()));
    }
    m_flagTabelle->resizeColumnsToContents();
}

void AnlageDetailDialog::slotBearbeiten()
{
    const Anlage* a = m_daten->findeAnlage(m_anlageId);
    if (!a) return;
    AnlageBearbeitenDialog dlg(*a, m_daten, m_benutzer, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_daten->anlageAendern(dlg.ergebnis());
        stammdatenAnzeigen();
        tagesAuswahlFuellen(m_tagCombo->currentData().toDate());
        tagLaden(m_tagCombo->currentData().toDate());
        trendFuellen();
        flagsFuellen();
    }
}

void AnlageDetailDialog::slotWartungErfassen()
{
    WartungDialog dlg(m_anlageId, m_daten, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_daten->wartungHinzufuegen(dlg.ergebnis());
        wartungenFuellen();
    }
}

void AnlageDetailDialog::slotBerichtCsv()
{
    const QString datei = QFileDialog::getSaveFileName(
        this,
        tr("Anlagenbericht als CSV speichern"),
        QString("Bericht_%1.csv").arg(m_anlageId),
        "CSV (*.csv)"
    );
    if (datei.isEmpty()) return;

    QFile f(datei);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Fehler"), f.errorString());
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "anlage_id,datum,verfuegbarkeit_pct,leistungsgrad_pct,qualitaetsrate_pct,oee_pct,gutteile,ausschuss,stoerungen,stoerzeit_min,mtbf_min,mttr_min\n";

    for (const OeeKennzahlen& dm : m_daten->alleTageskennzahlen(m_anlageId)) {
        out << m_anlageId << "," << dm.datum.toString("yyyy-MM-dd") << ","
            << QString::number(dm.verfuegbarkeit * 100.0, 'f', 2) << ","
            << QString::number(dm.leistungsgrad * 100.0, 'f', 2) << ","
            << QString::number(dm.qualitaet * 100.0, 'f', 2) << ","
            << QString::number(dm.oee * 100.0, 'f', 2) << ","
            << dm.gut << "," << dm.ausschuss << "," << dm.anzahlStoerungen << ","
            << QString::number(dm.stoerzeitSek / 60.0, 'f', 1) << ","
            << QString::number(dm.mtbfMin, 'f', 1) << ","
            << QString::number(dm.mttrMin, 'f', 1) << "\n";
    }
    QMessageBox::information(this, tr("Bericht gespeichert"),
                             tr("Der CSV-Bericht wurde gespeichert:\n%1").arg(datei));
}

void AnlageDetailDialog::slotBerichtPdf()
{
    const QString datei = QFileDialog::getSaveFileName(
        this,
        tr("Anlagenbericht als PDF speichern"),
        QString("Bericht_%1.pdf").arg(m_anlageId),
        "PDF (*.pdf)"
    );
    if (datei.isEmpty()) return;

    const Anlage* a = m_daten->findeAnlage(m_anlageId);
    if (!a) return;
    const QList<OeeKennzahlen> tage = m_daten->alleTageskennzahlen(m_anlageId);
    const Trendbericht b = m_daten->trendbericht(m_anlageId);

    QString html;
    html += QString("<h2>RoboCell - %1 (%2)</h2>").arg(a->anlageId.toHtmlEscaped(), a->bezeichnung.toHtmlEscaped());
    html += QString("<p><b>%1:</b> %2 (Pos. %3) | <b>%4:</b> %5 | <b>%6:</b> %7 s</p>")
                .arg(tr("Linie"), a->linieId.toHtmlEscaped()).arg(a->position)
                .arg(tr("Typ"), anlagenTypName(a->typ, a->typRoh).toHtmlEscaped())
                .arg(tr("Soll-Taktzeit")).arg(a->sollTaktzeitS, 0, 'f', 1);
    html += QString("<p><b>14-Tage-Mittelwert:</b> OEE = %1 % (V = %2 %, L = %3 %, Q = %4 %) | MTBF = %5 min | MTTR = %6 min</p>")
                .arg(b.mittelOee * 100.0, 0, 'f', 1)
                .arg(b.mittelVerfuegbarkeit * 100.0, 0, 'f', 1)
                .arg(b.mittelLeistung * 100.0, 0, 'f', 1)
                .arg(b.mittelQualitaet * 100.0, 0, 'f', 1)
                .arg(b.mtbfMin, 0, 'f', 1)
                .arg(b.mttrMin, 0, 'f', 1);

    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
    html += "<tr><th>Datum</th><th>V [%]</th><th>L [%]</th><th>Q [%]</th><th>OEE [%]</th><th>Gut</th><th>Ausschuss</th><th>Störzeit [min]</th></tr>";
    for (const OeeKennzahlen& dm : tage) {
        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td><b>%5</b></td><td>%6</td><td>%7</td><td>%8</td></tr>")
                    .arg(dm.datum.toString("yyyy-MM-dd"))
                    .arg(dm.gueltig ? QString::number(dm.verfuegbarkeit * 100.0, 'f', 1) : "-")
                    .arg(dm.gueltig ? QString::number(dm.leistungsgrad * 100.0, 'f', 1) : "-")
                    .arg(dm.gueltig ? QString::number(dm.qualitaet * 100.0, 'f', 1) : "-")
                    .arg(dm.gueltig ? QString::number(dm.oee * 100.0, 'f', 1) : "-")
                    .arg(dm.gut).arg(dm.ausschuss).arg(dm.stoerzeitSek / 60.0, 0, 'f', 1);
    }
    html += "</table>";

    QTextDocument doc;
    doc.setHtml(html);
    QPrinter drucker(QPrinter::HighResolution);
    drucker.setOutputFormat(QPrinter::PdfFormat);
    drucker.setOutputFileName(datei);
    doc.print(&drucker);

    QMessageBox::information(this, tr("PDF gespeichert"),
                             tr("Der PDF-Bericht wurde gespeichert:\n%1").arg(datei));
}
