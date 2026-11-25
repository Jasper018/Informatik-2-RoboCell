#include "farben.h"
#include <QPalette>
#include <cmath>

namespace Farben {

namespace {

struct PaletteDaten {
    QColor hintergrund;
    QColor text;
    QColor zebra;
    QColor hinweisText;
    QColor rahmen;
    QColor akzent;
    QColor akzentHover;
    QColor akzentText;
    QColor kopf;
    QColor kopfText;
    QColor deaktivHintergrund;
    QColor deaktivText;
    QColor tooltipHintergrund;
    QColor tooltipText;

    QColor okHintergrund;
    QColor okText;
    QColor warnHintergrund;
    QColor warnText;
    QColor fehlerHintergrund;
    QColor fehlerText;

    QColor balkenV;
    QColor balkenL;
    QColor balkenQ;
    QColor balkenOee;

    QColor produktion;
    QColor ruesten;
    QColor stoerung;
    QColor materialmangel;
    QColor rueckstau;
    QColor wartung;
    QColor pause;
    QColor unbekannt;
    QColor zustandText;

    QColor linieV;
    QColor linieL;
    QColor linieQ;
    QColor linieOee;
    QColor linieAusschuss;
};

const PaletteDaten& hellPalette()
{
    static const PaletteDaten p = {
        QColor("#ffffff"), QColor("#111111"), QColor("#f0f0f0"), QColor("#3d3d3d"),
        QColor("#424242"), QColor("#0b3d91"), QColor("#082a66"), QColor("#ffffff"),
        QColor("#1f2937"), QColor("#ffffff"), QColor("#e6e6e6"), QColor("#3a3a3a"),
        QColor("#1a1a1a"), QColor("#ffffff"),

        QColor("#d4f0d4"), QColor("#052e16"),
        QColor("#ffe8a6"), QColor("#3b2300"),
        QColor("#ffd0d0"), QColor("#4a0404"),

        QColor("#a8c8ff"), QColor("#ffd27a"), QColor("#9ee59e"), QColor("#c4c4c4"),

        QColor("#14532d"), QColor("#1e3a8a"), QColor("#7f1d1d"), QColor("#78350f"),
        QColor("#581c87"), QColor("#0b4a46"), QColor("#374151"), QColor("#000000"),
        QColor("#ffffff"),

        QColor("#0b3d91"), QColor("#78350f"), QColor("#14532d"), QColor("#111111"),
        QColor("#7f1d1d")
    };
    return p;
}

const PaletteDaten& dunkelPalette()
{
    static const PaletteDaten p = {
        QColor("#111318"), QColor("#f3f4f6"), QColor("#1b1f27"), QColor("#c4c9d4"),
        QColor("#b8c0cc"), QColor("#80bfff"), QColor("#a6d2ff"), QColor("#0a1628"),
        QColor("#232834"), QColor("#f9fafb"), QColor("#242933"), QColor("#c4c9d4"),
        QColor("#f3f4f6"), QColor("#111318"),

        QColor("#0b3b20"), QColor("#d1fae5"),
        QColor("#472900"), QColor("#fef3c7"),
        QColor("#4f0c0c"), QColor("#fee2e2"),

        QColor("#14376b"), QColor("#543000"), QColor("#0d3d21"), QColor("#374151"),

        QColor("#86efac"), QColor("#93c5fd"), QColor("#fca5a5"), QColor("#fcd34d"),
        QColor("#d8b4fe"), QColor("#5eead4"), QColor("#cbd5e1"), QColor("#f8fafc"),
        QColor("#0f172a"),

        QColor("#93c5fd"), QColor("#fcd34d"), QColor("#86efac"), QColor("#f3f4f6"),
        QColor("#fca5a5")
    };
    return p;
}

bool s_dunkel = false;

const PaletteDaten& aktiv()
{
    return s_dunkel ? dunkelPalette() : hellPalette();
}

} // namespace

bool dunkelmodus() { return s_dunkel; }
void setDunkelmodus(bool aktivModus) { s_dunkel = aktivModus; }

QColor hintergrund() { return aktiv().hintergrund; }
QColor text() { return aktiv().text; }
QColor zebra() { return aktiv().zebra; }
QColor hinweisText() { return aktiv().hinweisText; }
QColor rahmen() { return aktiv().rahmen; }
QColor akzent() { return aktiv().akzent; }
QColor akzentHover() { return aktiv().akzentHover; }
QColor akzentText() { return aktiv().akzentText; }
QColor kopf() { return aktiv().kopf; }
QColor kopfText() { return aktiv().kopfText; }
QColor deaktivHintergrund() { return aktiv().deaktivHintergrund; }
QColor deaktivText() { return aktiv().deaktivText; }
QColor tooltipHintergrund() { return aktiv().tooltipHintergrund; }
QColor tooltipText() { return aktiv().tooltipText; }

QColor okHintergrund() { return aktiv().okHintergrund; }
QColor okText() { return aktiv().okText; }
QColor warnHintergrund() { return aktiv().warnHintergrund; }
QColor warnText() { return aktiv().warnText; }
QColor fehlerHintergrund() { return aktiv().fehlerHintergrund; }
QColor fehlerText() { return aktiv().fehlerText; }

QColor balkenV() { return aktiv().balkenV; }
QColor balkenL() { return aktiv().balkenL; }
QColor balkenQ() { return aktiv().balkenQ; }
QColor balkenOee() { return aktiv().balkenOee; }

QColor linieV() { return aktiv().linieV; }
QColor linieL() { return aktiv().linieL; }
QColor linieQ() { return aktiv().linieQ; }
QColor linieOee() { return aktiv().linieOee; }
QColor linieAusschuss() { return aktiv().linieAusschuss; }

QColor zustandTextFarbe() { return aktiv().zustandText; }

QColor zustandFarbe(ZustandTyp typ)
{
    const PaletteDaten& p = aktiv();
    switch (typ) {
    case ZustandTyp::Produktion: return p.produktion;
    case ZustandTyp::Ruesten: return p.ruesten;
    case ZustandTyp::Stoerung: return p.stoerung;
    case ZustandTyp::Materialmangel: return p.materialmangel;
    case ZustandTyp::Rueckstau: return p.rueckstau;
    case ZustandTyp::Wartung: return p.wartung;
    case ZustandTyp::Pause: return p.pause;
    default: return p.unbekannt;
    }
}

double helligkeit(const QColor& farbe)
{
    const double kanaele[3] = {farbe.redF(), farbe.greenF(), farbe.blueF()};
    double linear[3];
    for (int i = 0; i < 3; i++) {
        const double c = kanaele[i];
        linear[i] = (c <= 0.03928) ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    }
    return 0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2];
}

double kontrast(const QColor& a, const QColor& b)
{
    double hell = helligkeit(a);
    double dunkel = helligkeit(b);
    if (hell < dunkel) {
        const double merker = hell;
        hell = dunkel;
        dunkel = merker;
    }
    return (hell + 0.05) / (dunkel + 0.05);
}

QList<Farbpaar> alleFarbpaare(bool dunkel)
{
    const PaletteDaten& p = dunkel ? dunkelPalette() : hellPalette();
    return {
        {"Text auf Hintergrund", p.text, p.hintergrund},
        {"Text auf Zebrazeile", p.text, p.zebra},
        {"Nebentext auf Hintergrund", p.hinweisText, p.hintergrund},
        {"Rahmen auf Hintergrund", p.rahmen, p.hintergrund},
        {"Rahmen auf Zebrazeile", p.rahmen, p.zebra},
        {"Button-Text auf Akzent", p.akzentText, p.akzent},
        {"Button-Text auf Akzent (Hover)", p.akzentText, p.akzentHover},
        {"Tabellenkopf", p.kopfText, p.kopf},
        {"Deaktivierter Text", p.deaktivText, p.deaktivHintergrund},
        {"Text auf Hover/Deaktiviert", p.text, p.deaktivHintergrund},
        {"Tooltip", p.tooltipText, p.tooltipHintergrund},
        {"Meldung OK", p.okText, p.okHintergrund},
        {"Meldung Warnung", p.warnText, p.warnHintergrund},
        {"Meldung Fehler", p.fehlerText, p.fehlerHintergrund},
        {"Balken Verfuegbarkeit", p.text, p.balkenV},
        {"Balken Leistungsgrad", p.text, p.balkenL},
        {"Balken Qualitaet", p.text, p.balkenQ},
        {"Balken OEE", p.text, p.balkenOee},
        {"Zustand Produktion", p.produktion, p.hintergrund},
        {"Zustand Ruesten", p.ruesten, p.hintergrund},
        {"Zustand Stoerung", p.stoerung, p.hintergrund},
        {"Zustand Materialmangel", p.materialmangel, p.hintergrund},
        {"Zustand Rueckstau", p.rueckstau, p.hintergrund},
        {"Zustand Wartung", p.wartung, p.hintergrund},
        {"Zustand Pause", p.pause, p.hintergrund},
        {"Zustand Unbekannt", p.unbekannt, p.hintergrund},
        {"Beschriftung auf Produktion", p.zustandText, p.produktion},
        {"Beschriftung auf Ruesten", p.zustandText, p.ruesten},
        {"Beschriftung auf Stoerung", p.zustandText, p.stoerung},
        {"Beschriftung auf Materialmangel", p.zustandText, p.materialmangel},
        {"Beschriftung auf Rueckstau", p.zustandText, p.rueckstau},
        {"Beschriftung auf Wartung", p.zustandText, p.wartung},
        {"Beschriftung auf Pause", p.zustandText, p.pause},
        {"Beschriftung auf Unbekannt", p.zustandText, p.unbekannt},
        {"Diagramm Verfuegbarkeit", p.linieV, p.hintergrund},
        {"Diagramm Leistungsgrad", p.linieL, p.hintergrund},
        {"Diagramm Qualitaet", p.linieQ, p.hintergrund},
        {"Diagramm OEE", p.linieOee, p.hintergrund},
        {"Diagramm Ausschuss", p.linieAusschuss, p.hintergrund}
    };
}

QList<Farbpaar> alleFarbpaare()
{
    return alleFarbpaare(s_dunkel);
}

QString styleSheet()
{
    const PaletteDaten& p = aktiv();
    const QString pfeilHoch = s_dunkel ? ":/icons/pfeil_hoch_hell.svg" : ":/icons/pfeil_hoch_dunkel.svg";
    const QString pfeilRunter = s_dunkel ? ":/icons/pfeil_runter_hell.svg" : ":/icons/pfeil_runter_dunkel.svg";
    const QString haken = s_dunkel ? ":/icons/haken_dunkel.svg" : ":/icons/haken_hell.svg";

    QString s;
    s += QString("QWidget { color: %1; background-color: %2; selection-background-color: %3; selection-color: %4; }")
             .arg(p.text.name(), p.hintergrund.name(), p.akzent.name(), p.akzentText.name());

    // Menueleiste und Menues
    s += QString("QMenuBar { background-color: %1; color: %2; border-bottom: 1px solid %3; }"
                 "QMenuBar::item { background-color: %1; color: %2; padding: 6px 12px; }"
                 "QMenuBar::item:selected { background-color: %4; color: %5; }")
             .arg(p.kopf.name(), p.kopfText.name(), p.rahmen.name(), p.akzent.name(), p.akzentText.name());
    s += QString("QMenu { background-color: %1; color: %2; border: 1px solid %3; }"
                 "QMenu::item { padding: 6px 28px 6px 20px; background-color: %1; color: %2; }"
                 "QMenu::item:selected { background-color: %4; color: %5; }"
                 "QMenu::item:disabled { background-color: %6; color: %7; }"
                 "QMenu::separator { height: 1px; background-color: %3; margin: 4px 8px; }")
             .arg(p.hintergrund.name(), p.text.name(), p.rahmen.name(), p.akzent.name(), p.akzentText.name(),
                  p.deaktivHintergrund.name(), p.deaktivText.name());

    // Buttons
    s += QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 4px; padding: 5px 14px; min-height: 22px; }"
                 "QPushButton:hover { background-color: %4; }"
                 "QPushButton:focus { border: 2px solid %5; }"
                 "QPushButton:disabled { background-color: %6; color: %7; border: 1px dashed %3; }")
             .arg(p.akzent.name(), p.akzentText.name(), p.rahmen.name(), p.akzentHover.name(), p.text.name(),
                  p.deaktivHintergrund.name(), p.deaktivText.name());

    // Textfelder
    s += QString("QLineEdit, QPlainTextEdit, QTextEdit {"
                 " background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px; padding: 4px 6px; min-height: 22px; }"
                 "QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus { border: 2px solid %4; }"
                 "QLineEdit:disabled, QPlainTextEdit:disabled, QTextEdit:disabled { background-color: %5; color: %6; }")
             .arg(p.hintergrund.name(), p.text.name(), p.rahmen.name(), p.akzent.name(),
                  p.deaktivHintergrund.name(), p.deaktivText.name());

    // Auswahlfelder (ComboBox) mit sauberem Dropdown-Pfeil
    s += QString("QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px;"
                 " padding: 4px 28px 4px 8px; min-height: 22px; }"
                 "QComboBox:focus { border: 2px solid %4; }"
                 "QComboBox:disabled { background-color: %5; color: %6; }"
                 "QComboBox::drop-down { subcontrol-origin: border; subcontrol-position: top right; width: 24px;"
                 " border-left: 1px solid %3; background-color: %7; border-top-right-radius: 3px; border-bottom-right-radius: 3px; }"
                 "QComboBox::down-arrow { image: url(%8); width: 11px; height: 7px; }"
                 "QComboBox QAbstractItemView { background-color: %1; color: %2; border: 1px solid %3;"
                 " selection-background-color: %4; selection-color: %9; }")
             .arg(p.hintergrund.name(), p.text.name(), p.rahmen.name(), p.akzent.name(),
                  p.deaktivHintergrund.name(), p.deaktivText.name(), p.zebra.name(),
                  pfeilRunter, p.akzentText.name());

    // Zahlen- und Datumsfelder (SpinBox, DoubleSpinBox, DateEdit) ohne abgeschnittene Pfeile
    s += QString("QSpinBox, QDoubleSpinBox, QDateEdit {"
                 " background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px;"
                 " padding: 4px 26px 4px 6px; min-height: 26px; }"
                 "QSpinBox:focus, QDoubleSpinBox:focus, QDateEdit:focus { border: 2px solid %4; }"
                 "QSpinBox:disabled, QDoubleSpinBox:disabled, QDateEdit:disabled { background-color: %5; color: %6; }"
                 "QSpinBox::up-button, QDoubleSpinBox::up-button, QDateEdit::up-button {"
                 " subcontrol-origin: border; subcontrol-position: top right; width: 22px;"
                 " border-left: 1px solid %3; border-bottom: 1px solid %3; background-color: %7; border-top-right-radius: 3px; }"
                 "QSpinBox::down-button, QDoubleSpinBox::down-button, QDateEdit::down-button {"
                 " subcontrol-origin: border; subcontrol-position: bottom right; width: 22px;"
                 " border-left: 1px solid %3; background-color: %7; border-bottom-right-radius: 3px; }"
                 "QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover, QDateEdit::up-button:hover,"
                 " QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover, QDateEdit::down-button:hover {"
                 " background-color: %5; }"
                 "QSpinBox::up-arrow, QDoubleSpinBox::up-arrow, QDateEdit::up-arrow { image: url(%8); width: 10px; height: 6px; }"
                 "QSpinBox::down-arrow, QDoubleSpinBox::down-arrow, QDateEdit::down-arrow { image: url(%9); width: 10px; height: 6px; }"
                 "QDateEdit::drop-down { subcontrol-origin: border; subcontrol-position: top right; width: 24px;"
                 " border-left: 1px solid %3; background-color: %7; border-top-right-radius: 3px; border-bottom-right-radius: 3px; }")
             .arg(p.hintergrund.name(), p.text.name(), p.rahmen.name(), p.akzent.name(),
                  p.deaktivHintergrund.name(), p.deaktivText.name(), p.zebra.name(),
                  pfeilHoch, pfeilRunter);

    // Tabellen und Listen
    s += QString("QTableView, QTableWidget, QListWidget, QTreeWidget { background-color: %1; color: %2;"
                 " alternate-background-color: %3; gridline-color: %4; border: 1px solid %4; }"
                 "QHeaderView::section { background-color: %5; color: %6; padding: 6px; border: 1px solid %4; font-weight: bold; }"
                 "QTableCornerButton::section { background-color: %5; border: 1px solid %4; }")
             .arg(p.hintergrund.name(), p.text.name(), p.zebra.name(), p.rahmen.name(), p.kopf.name(), p.kopfText.name());

    // Registerkarten
    s += QString("QTabWidget::pane { border: 1px solid %1; }"
                 "QTabBar::tab { background-color: %2; color: %3; border: 1px solid %1; padding: 7px 14px; margin-right: 2px; }"
                 "QTabBar::tab:selected { background-color: %4; color: %5; font-weight: bold; }"
                 "QTabBar::tab:hover:!selected { background-color: %6; }")
             .arg(p.rahmen.name(), p.zebra.name(), p.text.name(), p.akzent.name(), p.akzentText.name(),
                  p.deaktivHintergrund.name());

    // Gruppen, Statusleiste, Tooltips, Fortschrittsbalken
    s += QString("QGroupBox { border: 1px solid %1; border-radius: 4px; margin-top: 14px; padding: 8px 6px 6px 6px; }"
                 "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; font-weight: bold; }")
             .arg(p.rahmen.name());
    s += QString("QStatusBar { background-color: %1; color: %2; border-top: 1px solid %3; }"
                 "QStatusBar QLabel { background-color: %1; color: %2; padding: 2px 6px; }")
             .arg(p.zebra.name(), p.text.name(), p.rahmen.name());
    s += QString("QToolTip { background-color: %1; color: %2; border: 1px solid %2; padding: 5px; }")
             .arg(p.tooltipHintergrund.name(), p.tooltipText.name());
    s += QString("QProgressBar { border: 1px solid %1; border-radius: 3px; background-color: %2; color: %3; text-align: center; font-weight: bold; }"
                 "QProgressBar::chunk { background-color: %4; }")
             .arg(p.rahmen.name(), p.hintergrund.name(), p.text.name(), p.balkenV.name());

    // Meldungsfelder fuer OK, Warnung und Fehler (keine blauen Infokaestchen)
    const QString rahmenForm = "border: 1px solid %2; border-radius: 4px; padding: 6px 8px; background-color: %1; color: %2;";
    s += "QLabel[meldung=\"ok\"] { " + rahmenForm.arg(p.okHintergrund.name(), p.okText.name()) + " }";
    s += "QLabel[meldung=\"warn\"] { " + rahmenForm.arg(p.warnHintergrund.name(), p.warnText.name()) + " }";
    s += "QLabel[meldung=\"fehler\"] { " + rahmenForm.arg(p.fehlerHintergrund.name(), p.fehlerText.name()) + " }";

    s += QString("QLabel[hinweis=\"true\"] { color: %1; }").arg(p.hinweisText.name());

    // Kennzahlbalken
    s += QString("QProgressBar#pb_v::chunk { background-color: %1; }"
                 "QProgressBar#pb_l::chunk { background-color: %2; }"
                 "QProgressBar#pb_q::chunk { background-color: %3; }"
                 "QProgressBar#pb_oee::chunk { background-color: %4; }")
             .arg(p.balkenV.name(), p.balkenL.name(), p.balkenQ.name(), p.balkenOee.name());

    // Kontrollkaestchen mit sichtbarem Haekchen
    s += QString("QCheckBox { color: %1; spacing: 7px; min-height: 22px; }"
                 "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid %2; border-radius: 3px; background-color: %3; }"
                 "QCheckBox::indicator:checked { background-color: %4; border: 1px solid %1; image: url(%5); }"
                 "QCheckBox:disabled { color: %6; }")
             .arg(p.text.name(), p.rahmen.name(), p.hintergrund.name(), p.akzent.name(), haken, p.deaktivText.name());

    // Moderne Scrollleisten ohne abgeschnittene Minipfeile
    s += QString("QScrollBar:vertical { background: %1; width: 12px; margin: 0px; border: 1px solid %2; border-radius: 6px; }"
                 "QScrollBar::handle:vertical { background: %2; min-height: 28px; border-radius: 5px; margin: 1px; }"
                 "QScrollBar::handle:vertical:hover { background: %3; }"
                 "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                 "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
                 "QScrollBar:horizontal { background: %1; height: 12px; margin: 0px; border: 1px solid %2; border-radius: 6px; }"
                 "QScrollBar::handle:horizontal { background: %2; min-width: 28px; border-radius: 5px; margin: 1px; }"
                 "QScrollBar::handle:horizontal:hover { background: %3; }"
                 "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
                 "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }")
             .arg(p.zebra.name(), p.rahmen.name(), p.text.name());

    // Schieberegler mit ausreichend Hoehe, damit der Griff nicht abgeschnitten wird
    s += QString("QSlider::groove:horizontal { height: 6px; background: %1; border: 1px solid %2; border-radius: 3px; }"
                 "QSlider::sub-page:horizontal { background: %3; border: 1px solid %2; border-radius: 3px; height: 6px; }"
                 "QSlider::handle:horizontal { background: %3; border: 2px solid %4; width: 16px; height: 16px; margin: -6px 0; border-radius: 9px; }"
                 "QSlider::handle:horizontal:hover { background: %5; }"
                 "QSlider::handle:horizontal:disabled { background: %1; border: 1px solid %2; }"
                 "QSlider:horizontal { min-height: 26px; }")
             .arg(p.deaktivHintergrund.name(), p.rahmen.name(), p.akzent.name(), p.text.name(), p.akzentHover.name());

    s += QString("QSplitter::handle { background-color: %1; }").arg(p.zebra.name());
    return s;
}

QPalette anwendungsPalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, hintergrund());
    pal.setColor(QPalette::WindowText, text());
    pal.setColor(QPalette::Base, hintergrund());
    pal.setColor(QPalette::AlternateBase, zebra());
    pal.setColor(QPalette::Text, text());
    pal.setColor(QPalette::PlaceholderText, hinweisText());
    pal.setColor(QPalette::Button, akzent());
    pal.setColor(QPalette::ButtonText, akzentText());
    pal.setColor(QPalette::Highlight, akzent());
    pal.setColor(QPalette::HighlightedText, akzentText());
    return pal;
}

} // namespace Farben
