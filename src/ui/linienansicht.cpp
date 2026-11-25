#include "linienansicht.h"
#include "core/farben.h"
#include "io/datenverwaltung.h"
#include "logic/oeeberechnung.h"
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QSlider>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>

LinienAnsichtWidget::LinienAnsichtWidget(Datenverwaltung* daten, QWidget* parent)
    : QWidget(parent)
    , m_daten(daten)
{
    QVBoxLayout* haupt = new QVBoxLayout(this);
    haupt->setSpacing(8);

    // Steuerleiste zweizeilig im Grid, passt auch auf kleinere Bildschirme
    QGridLayout* steuer = new QGridLayout();
    steuer->setSpacing(6);

    m_linieLabel = new QLabel(this);
    m_linieCombo = new QComboBox(this);
    m_tagLabel = new QLabel(this);
    m_tagCombo = new QComboBox(this);
    m_modusLabel = new QLabel(this);
    m_modusCombo = new QComboBox(this);
    m_uhrzeitLabel = new QLabel(this);

    m_zeitSlider = new QSlider(Qt::Horizontal, this);
    m_zeitSlider->setRange(0, 24 * 60 - 1);   // 06:00 bis 05:59 Folgetag
    m_zeitSlider->setValue(180);               // 09:00 Uhr
    m_zeitSlider->setEnabled(false);

    m_uhrzeitWertLabel = new QLabel("09:00", this);
    QFont uhrSchrift = font();
    uhrSchrift.setBold(true);
    m_uhrzeitWertLabel->setFont(uhrSchrift);
    m_uhrzeitWertLabel->setMinimumWidth(48);

    steuer->addWidget(m_linieLabel, 0, 0);
    steuer->addWidget(m_linieCombo, 0, 1);
    steuer->addWidget(m_tagLabel, 0, 2);
    steuer->addWidget(m_tagCombo, 0, 3);
    steuer->addWidget(m_modusLabel, 0, 4);
    steuer->addWidget(m_modusCombo, 0, 5);

    steuer->addWidget(m_uhrzeitLabel, 1, 0);
    steuer->addWidget(m_zeitSlider, 1, 1, 1, 4);
    steuer->addWidget(m_uhrzeitWertLabel, 1, 5);
    steuer->setColumnStretch(1, 2);
    steuer->setColumnStretch(3, 1);
    steuer->setColumnStretch(5, 2);
    haupt->addLayout(steuer);

    m_engpassBanner = new QLabel(this);
    m_engpassBanner->setWordWrap(true);
    m_engpassBanner->setProperty("meldung", "warn");
    haupt->addWidget(m_engpassBanner);

    QSplitter* teiler = new QSplitter(Qt::Vertical, this);
    teiler->setChildrenCollapsible(false);

    m_szene = new QGraphicsScene(this);
    m_szene->setBackgroundBrush(Farben::HINTERGRUND);
    m_szene->installEventFilter(this);

    m_ansicht = new QGraphicsView(m_szene, this);
    m_ansicht->setRenderHint(QPainter::Antialiasing, true);
    m_ansicht->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ansicht->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_ansicht->setMinimumHeight(180);
    teiler->addWidget(m_ansicht);

    m_engpassTabelle = new QTableWidget(this);
    m_engpassTabelle->setColumnCount(7);
    m_engpassTabelle->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_engpassTabelle->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_engpassTabelle->setAlternatingRowColors(true);
    m_engpassTabelle->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_engpassTabelle->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_engpassTabelle->horizontalHeader()->setStretchLastSection(true);
    m_engpassTabelle->verticalHeader()->setVisible(false);
    teiler->addWidget(m_engpassTabelle);

    teiler->setStretchFactor(0, 3);
    teiler->setStretchFactor(1, 2);
    haupt->addWidget(teiler, 1);

    connect(m_linieCombo, &QComboBox::currentIndexChanged, this, &LinienAnsichtWidget::slotAuswahlGeaendert);
    connect(m_tagCombo, &QComboBox::currentIndexChanged, this, &LinienAnsichtWidget::slotAuswahlGeaendert);
    connect(m_modusCombo, &QComboBox::currentIndexChanged, this, &LinienAnsichtWidget::slotAuswahlGeaendert);
    connect(m_zeitSlider, &QSlider::valueChanged, this, &LinienAnsichtWidget::slotZeitGeaendert);
    connect(m_engpassTabelle, &QTableWidget::cellDoubleClicked, this, [this](int zeile, int) {
        if (QTableWidgetItem* it = m_engpassTabelle->item(zeile, 0)) {
            emit anlageDoppelklick(it->text());
        }
    });

    texteAktualisieren();
}

void LinienAnsichtWidget::changeEvent(QEvent* ereignis)
{
    if (ereignis->type() == QEvent::LanguageChange) {
        texteAktualisieren();
    }
    QWidget::changeEvent(ereignis);
}

void LinienAnsichtWidget::texteAktualisieren()
{
    m_linieLabel->setText(tr("Fertigungslinie:"));
    m_tagLabel->setText(tr("Tag:"));
    m_modusLabel->setText(tr("Einfärbung:"));
    m_uhrzeitLabel->setText(tr("Uhrzeit (Zustand):"));

    const int alterModus = std::max(0, m_modusCombo->currentIndex());
    m_modusCombo->blockSignals(true);
    m_modusCombo->clear();
    m_modusCombo->addItem(tr("Nach Tages-OEE"));
    m_modusCombo->addItem(tr("Nach Zustand zur Uhrzeit"));
    m_modusCombo->addItem(tr("Nach Engpass-Bewertung"));
    m_modusCombo->setCurrentIndex(alterModus);
    m_modusCombo->blockSignals(false);

    m_engpassTabelle->setHorizontalHeaderLabels({
        tr("Anlage-ID"), tr("Pos."), tr("Bezeichnung"),
        tr("Soll-Takt"), tr("Eigene Störzeit"),
        tr("Verursachter Mangel / Rückstau"), tr("Engpass-Bewertung")
    });

    szeneZeichnen();
}

void LinienAnsichtWidget::aktualisieren()
{
    if (!m_daten) return;

    const QString alteLinie = m_linieCombo->currentData().toString();
    const QDate alterTag = m_tagCombo->currentData().toDate();

    m_linieCombo->blockSignals(true);
    m_linieCombo->clear();
    for (const Linie& l : m_daten->linien()) {
        m_linieCombo->addItem(QString("%1 - %2 (%3)").arg(l.linieId, l.bezeichnung, l.halle), l.linieId);
    }
    const int lIdx = m_linieCombo->findData(alteLinie);
    if (lIdx >= 0) m_linieCombo->setCurrentIndex(lIdx);
    m_linieCombo->blockSignals(false);

    m_tagCombo->blockSignals(true);
    m_tagCombo->clear();
    for (const QDate& d : m_daten->alleTage()) {
        m_tagCombo->addItem(d.toString("yyyy-MM-dd"), d);
    }
    const int dIdx = m_tagCombo->findData(alterTag);
    if (dIdx >= 0) m_tagCombo->setCurrentIndex(dIdx);
    m_tagCombo->blockSignals(false);

    szeneZeichnen();
}

void LinienAnsichtWidget::slotAuswahlGeaendert()
{
    m_zeitSlider->setEnabled(m_modusCombo->currentIndex() == 1);
    szeneZeichnen();
}

void LinienAnsichtWidget::slotZeitGeaendert(int minutenAbSechs)
{
    const QDate tag = m_tagCombo->currentData().toDate();
    const QDateTime basis(tag.isValid() ? tag : QDate(2026, 9, 14), QTime(6, 0, 0));
    m_uhrzeitWertLabel->setText(basis.addSecs(minutenAbSechs * 60).toString("HH:mm"));
    if (m_modusCombo->currentIndex() == 1) {
        szeneZeichnen();
    }
}

void LinienAnsichtWidget::szeneZeichnen()
{
    m_szene->clear();
    m_engpassTabelle->setRowCount(0);
    if (!m_daten || m_linieCombo->count() == 0) {
        m_engpassBanner->setText(tr("Keine Linien geladen."));
        return;
    }

    const QString linieId = m_linieCombo->currentData().toString();
    const QDate tag = m_tagCombo->currentData().toDate();
    const int modus = m_modusCombo->currentIndex();

    QList<Anlage> stationen;
    for (const Anlage& a : m_daten->anlagen()) {
        if (a.linieId == linieId) stationen.append(a);
    }
    std::sort(stationen.begin(), stationen.end(), [](const Anlage& x, const Anlage& y) {
        return x.position < y.position;
    });

    const QList<EngpassEintrag> engpaesse = m_daten->engpass(linieId, tag);
    QMap<QString, EngpassEintrag> engpassMap;
    QString hauptId;
    for (const EngpassEintrag& e : engpaesse) {
        engpassMap.insert(e.anlageId, e);
        if (e.hauptEngpass) hauptId = e.anlageId;
    }

    if (!hauptId.isEmpty()) {
        const EngpassEintrag& pb = engpassMap[hauptId];
        m_engpassBanner->setText(tr("Haupt-Engpass der Linie %1 am %2: %3 (%4) - "
                                    "Soll-Takt: %5 s, Störzeit: %6 min, Nachbarverluste: %7 min. "
                                    "Doppelklick auf einen Kasten öffnet die Anlage.")
                                     .arg(linieId, tag.toString("dd.MM.yyyy"), pb.anlageId, pb.bezeichnung)
                                     .arg(pb.sollTaktzeitS, 0, 'f', 1)
                                     .arg(pb.stoerzeitMin, 0, 'f', 1)
                                     .arg(pb.verursachterMangelMin + pb.verursachterRueckstauMin, 0, 'f', 1));
    } else {
        m_engpassBanner->setText(tr("Keine Stationen in dieser Linie vorhanden."));
    }

    // Engpass-Tabelle fuellen
    m_engpassTabelle->setRowCount(engpaesse.size());
    for (int r = 0; r < engpaesse.size(); r++) {
        const EngpassEintrag& b = engpaesse[r];
        auto zelle = [&](const QString& txt) {
            QTableWidgetItem* it = new QTableWidgetItem(txt);
            if (b.hauptEngpass) {
                it->setBackground(Farben::WARN_HINTERGRUND);
                it->setForeground(Farben::WARN_TEXT);
                QFont f = it->font();
                f.setBold(true);
                it->setFont(f);
            }
            return it;
        };
        m_engpassTabelle->setItem(r, 0, zelle(b.anlageId));
        m_engpassTabelle->setItem(r, 1, zelle(QString::number(b.position)));
        m_engpassTabelle->setItem(r, 2, zelle(b.bezeichnung));
        m_engpassTabelle->setItem(r, 3, zelle(QString("%1 s").arg(b.sollTaktzeitS, 0, 'f', 1)));
        m_engpassTabelle->setItem(r, 4, zelle(QString("%1 min").arg(b.stoerzeitMin, 0, 'f', 1)));
        m_engpassTabelle->setItem(r, 5, zelle(QString("%1 / %2 min")
            .arg(b.verursachterMangelMin, 0, 'f', 1)
            .arg(b.verursachterRueckstauMin, 0, 'f', 1)));
        m_engpassTabelle->setItem(r, 6, zelle(b.hauptEngpass
            ? tr("Haupt-Engpass (%1)").arg(b.score, 0, 'f', 1)
            : QString::number(b.score, 'f', 1)));
    }
    m_engpassTabelle->resizeColumnsToContents();

    // Kaesten in Materialflussrichtung zeichnen (hoher Kontrast: weisser Kasten, farbiger Kopf)
    const int boxW = 165;
    const int boxH = 92;
    const int kopfH = 24;
    const int lueckeX = 34;
    const int lueckeY = 38;
    const int jeReihe = 6;
    const QDateTime abfrageZeit = QDateTime(tag, QTime(6, 0, 0)).addSecs(m_zeitSlider->value() * 60);

    for (int i = 0; i < stationen.size(); i++) {
        const Anlage& a = stationen[i];
        const int reihe = i / jeReihe;
        const int spalte = i % jeReihe;
        const double x = 16 + spalte * (boxW + lueckeX);
        const double y = 16 + reihe * (boxH + lueckeY);

        const OeeKennzahlen dm = m_daten->tageskennzahlen(a.anlageId, tag);
        QColor kopfBg = Farben::KOPF;
        QColor kopfFg = Farben::KOPF_TEXT;
        QString statusZeile;

        if (modus == 0) {
            if (!dm.gueltig) {
                kopfBg = Farben::ZEBRA;
                kopfFg = Farben::TEXT;
                statusZeile = tr("Keine Schicht");
            } else {
                if (dm.oee >= 0.80) { kopfBg = Farben::OK_HINTERGRUND; kopfFg = Farben::OK_TEXT; }
                else if (dm.oee >= 0.70) { kopfBg = Farben::WARN_HINTERGRUND; kopfFg = Farben::WARN_TEXT; }
                else { kopfBg = Farben::FEHLER_HINTERGRUND; kopfFg = Farben::FEHLER_TEXT; }
                statusZeile = QString("OEE: %1 %  |  V: %2 %")
                    .arg(dm.oee * 100.0, 0, 'f', 1)
                    .arg(dm.verfuegbarkeit * 100.0, 0, 'f', 0);
            }
        } else if (modus == 1) {
            std::shared_ptr<Schichtprotokoll> p = m_daten->protokoll(a.anlageId, tag);
            ZustandTyp aktiverTyp = ZustandTyp::Unbekannt;
            bool gefunden = false;
            if (p && p->geladen) {
                for (const Schicht& s : p->schichten) {
                    if (!s.geplant) continue;
                    for (const ZustandEintrag& z : s.zustaende) {
                        if (z.beginn <= abfrageZeit && abfrageZeit < z.ende) {
                            aktiverTyp = z.typ;
                            gefunden = true;
                            break;
                        }
                    }
                    if (gefunden) break;
                }
            }
            if (gefunden) {
                kopfBg = Farben::zustandFarbe(aktiverTyp);
                kopfFg = Farben::KOPF_TEXT;
                statusZeile = zustandName(aktiverTyp);
            } else {
                kopfBg = Farben::ZEBRA;
                kopfFg = Farben::TEXT;
                statusZeile = tr("Schichtfrei");
            }
        } else {
            const EngpassEintrag bn = engpassMap.value(a.anlageId);
            if (bn.hauptEngpass) {
                kopfBg = Farben::STOERUNG;
                kopfFg = Farben::KOPF_TEXT;
                statusZeile = tr("Engpass (%1 s)").arg(a.sollTaktzeitS, 0, 'f', 1);
            } else {
                kopfBg = Farben::INFO_HINTERGRUND;
                kopfFg = Farben::INFO_TEXT;
                statusZeile = tr("Score: %1").arg(bn.score, 0, 'f', 1);
            }
        }

        // Pfeil zur naechsten Station in derselben Reihe
        if (i + 1 < stationen.size() && (i + 1) / jeReihe == reihe) {
            const double px1 = x + boxW;
            const double py = y + boxH / 2.0;
            const double px2 = px1 + lueckeX;
            m_szene->addLine(px1, py, px2, py, QPen(Farben::TEXT, 2));
            QPolygonF spitze;
            spitze << QPointF(px2, py) << QPointF(px2 - 8, py - 5) << QPointF(px2 - 8, py + 5);
            m_szene->addPolygon(spitze, QPen(Farben::TEXT), QBrush(Farben::TEXT));
        }

        const bool istEngpass = engpassMap.value(a.anlageId).hauptEngpass;
        QGraphicsRectItem* kasten = m_szene->addRect(
            QRectF(x, y, boxW, boxH),
            QPen(istEngpass ? Farben::STOERUNG : Farben::RAHMEN, istEngpass ? 3.0 : 1.5),
            QBrush(Farben::HINTERGRUND)
        );
        kasten->setData(0, a.anlageId);
        kasten->setCursor(Qt::PointingHandCursor);
        kasten->setToolTip(tr("Doppelklick zum Öffnen: %1 (%2)").arg(a.anlageId, a.bezeichnung));

        QGraphicsRectItem* kopf = m_szene->addRect(
            QRectF(x, y, boxW, kopfH),
            QPen(istEngpass ? Farben::STOERUNG : Farben::RAHMEN, 1.0),
            QBrush(kopfBg)
        );
        kopf->setData(0, a.anlageId);
        kopf->setCursor(Qt::PointingHandCursor);

        QGraphicsTextItem* kopfTxt = m_szene->addText(QString("Pos %1  |  %2").arg(a.position).arg(a.anlageId));
        kopfTxt->setPos(x + 6, y + 1);
        kopfTxt->setDefaultTextColor(kopfFg);
        QFont fett = font();
        fett.setBold(true);
        fett.setPointSize(std::max(8, font().pointSize() - 1));
        kopfTxt->setFont(fett);
        kopfTxt->setData(0, a.anlageId);

        QString kurzName = a.bezeichnung;
        if (kurzName.size() > 21) kurzName = kurzName.left(19) + "..";
        QGraphicsTextItem* inhaltTxt = m_szene->addText(
            QString("%1\n%2\nTakt: %3 s")
                .arg(kurzName, statusZeile)
                .arg(a.sollTaktzeitS, 0, 'f', 1)
        );
        inhaltTxt->setPos(x + 6, y + kopfH + 4);
        inhaltTxt->setDefaultTextColor(Farben::TEXT);
        QFont klein = font();
        klein.setPointSize(std::max(8, font().pointSize() - 1));
        inhaltTxt->setFont(klein);
        inhaltTxt->setData(0, a.anlageId);
    }

    m_szene->setSceneRect(m_szene->itemsBoundingRect().adjusted(-14, -14, 14, 14));
}

bool LinienAnsichtWidget::eventFilter(QObject* objekt, QEvent* ereignis)
{
    if (objekt == m_szene && ereignis->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        QGraphicsSceneMouseEvent* maus = static_cast<QGraphicsSceneMouseEvent*>(ereignis);
        const QList<QGraphicsItem*> treffer = m_szene->items(maus->scenePos());
        for (QGraphicsItem* item : treffer) {
            const QString id = item->data(0).toString();
            if (!id.isEmpty()) {
                emit anlageDoppelklick(id);
                return true;
            }
        }
    }
    return QWidget::eventFilter(objekt, ereignis);
}
