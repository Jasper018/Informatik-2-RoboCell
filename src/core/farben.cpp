#include "farben.h"
#include <cmath>

namespace Farben {

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

QList<Farbpaar> alleFarbpaare()
{
    return {
        {"Text auf Hintergrund", TEXT, HINTERGRUND},
        {"Text auf Zebrazeile", TEXT, ZEBRA},
        {"Nebentext auf Hintergrund", HINWEIS_TEXT, HINTERGRUND},
        {"Rahmen auf Hintergrund", RAHMEN, HINTERGRUND},
        {"Button-Text auf Akzent", AKZENT_TEXT, AKZENT},
        {"Button-Text auf Akzent (Hover)", AKZENT_TEXT, AKZENT_HOVER},
        {"Tabellenkopf", KOPF_TEXT, KOPF},
        {"Deaktivierter Text", DEAKTIV_TEXT, DEAKTIV_HINTERGRUND},
        {"Text auf Hover/Deaktiviert", TEXT, DEAKTIV_HINTERGRUND},
        {"Tooltip", TOOLTIP_TEXT, TOOLTIP_HINTERGRUND},
        {"Meldung OK", OK_TEXT, OK_HINTERGRUND},
        {"Meldung Warnung", WARN_TEXT, WARN_HINTERGRUND},
        {"Meldung Fehler", FEHLER_TEXT, FEHLER_HINTERGRUND},
        {"Meldung Info", INFO_TEXT, INFO_HINTERGRUND},
        {"Balken Verfuegbarkeit", TEXT, BALKEN_V},
        {"Balken Leistungsgrad", TEXT, BALKEN_L},
        {"Balken Qualitaet", TEXT, BALKEN_Q},
        {"Balken OEE", TEXT, BALKEN_OEE},
        {"Zustand Produktion", PRODUKTION, HINTERGRUND},
        {"Zustand Ruesten", RUESTEN, HINTERGRUND},
        {"Zustand Stoerung", STOERUNG, HINTERGRUND},
        {"Zustand Materialmangel", MATERIALMANGEL, HINTERGRUND},
        {"Zustand Rueckstau", RUECKSTAU, HINTERGRUND},
        {"Zustand Wartung", WARTUNG, HINTERGRUND},
        {"Zustand Pause", PAUSE, HINTERGRUND},
        {"Zustand Unbekannt", UNBEKANNT, HINTERGRUND},
        {"Beschriftung auf Produktion", KOPF_TEXT, PRODUKTION},
        {"Beschriftung auf Ruesten", KOPF_TEXT, RUESTEN},
        {"Beschriftung auf Stoerung", KOPF_TEXT, STOERUNG},
        {"Beschriftung auf Materialmangel", KOPF_TEXT, MATERIALMANGEL},
        {"Beschriftung auf Rueckstau", KOPF_TEXT, RUECKSTAU},
        {"Beschriftung auf Wartung", KOPF_TEXT, WARTUNG},
        {"Beschriftung auf Pause", KOPF_TEXT, PAUSE},
        {"Beschriftung auf Unbekannt", KOPF_TEXT, UNBEKANNT},
        {"Diagramm Verfuegbarkeit", LINIE_V, HINTERGRUND},
        {"Diagramm Leistungsgrad", LINIE_L, HINTERGRUND},
        {"Diagramm Qualitaet", LINIE_Q, HINTERGRUND},
        {"Diagramm OEE", LINIE_OEE, HINTERGRUND},
        {"Diagramm Ausschuss", LINIE_AUSSCHUSS, HINTERGRUND}
    };
}

QString styleSheet()
{
    // Die Platzhalter %1 usw. werden der Reihe nach durch Farben ersetzt
    QString s;
    s += QString("QWidget { color: %1; background-color: %2; selection-background-color: %3; selection-color: %4; }")
             .arg(TEXT.name(), HINTERGRUND.name(), AKZENT.name(), AKZENT_TEXT.name());

    // Menueleiste und Menues
    s += QString("QMenuBar { background-color: %1; color: %2; }"
                 "QMenuBar::item { background-color: %1; color: %2; padding: 6px 12px; }"
                 "QMenuBar::item:selected { background-color: %3; color: %2; }")
             .arg(KOPF.name(), KOPF_TEXT.name(), AKZENT.name());
    s += QString("QMenu { background-color: %1; color: %2; border: 1px solid %3; }"
                 "QMenu::item { padding: 5px 28px 5px 20px; background-color: %1; color: %2; }"
                 "QMenu::item:selected { background-color: %4; color: %5; }"
                 "QMenu::item:disabled { background-color: %6; color: %7; }"
                 "QMenu::separator { height: 1px; background-color: %3; margin: 4px 8px; }")
             .arg(HINTERGRUND.name(), TEXT.name(), RAHMEN.name(), AKZENT.name(), AKZENT_TEXT.name(),
                  DEAKTIV_HINTERGRUND.name(), DEAKTIV_TEXT.name());

    // Buttons
    s += QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: 3px; padding: 5px 14px; }"
                 "QPushButton:hover { background-color: %4; }"
                 "QPushButton:focus { border: 2px solid %5; }"
                 "QPushButton:disabled { background-color: %6; color: %7; border: 1px dashed %3; }")
             .arg(AKZENT.name(), AKZENT_TEXT.name(), KOPF.name(), AKZENT_HOVER.name(), TEXT.name(),
                  DEAKTIV_HINTERGRUND.name(), DEAKTIV_TEXT.name());

    // Eingabefelder
    s += QString("QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit, QPlainTextEdit, QTextEdit {"
                 " background-color: %1; color: %2; border: 1px solid %3; padding: 3px; }"
                 "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus, QDateEdit:focus {"
                 " border: 2px solid %4; }"
                 "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled, QDateEdit:disabled {"
                 " background-color: %5; color: %6; }"
                 "QComboBox QAbstractItemView { background-color: %1; color: %2; border: 1px solid %3;"
                 " selection-background-color: %4; selection-color: %7; }")
             .arg(HINTERGRUND.name(), TEXT.name(), RAHMEN.name(), AKZENT.name(),
                  DEAKTIV_HINTERGRUND.name(), DEAKTIV_TEXT.name(), AKZENT_TEXT.name());

    // Tabellen und Listen
    s += QString("QTableView, QTableWidget, QListWidget, QTreeWidget { background-color: %1; color: %2;"
                 " alternate-background-color: %3; gridline-color: %4; border: 1px solid %4; }"
                 "QHeaderView::section { background-color: %5; color: %6; padding: 5px; border: 1px solid %4; }"
                 "QTableCornerButton::section { background-color: %5; border: 1px solid %4; }")
             .arg(HINTERGRUND.name(), TEXT.name(), ZEBRA.name(), RAHMEN.name(), KOPF.name(), KOPF_TEXT.name());

    // Registerkarten
    s += QString("QTabWidget::pane { border: 1px solid %1; }"
                 "QTabBar::tab { background-color: %2; color: %3; border: 1px solid %1; padding: 7px 14px; margin-right: 2px; }"
                 "QTabBar::tab:selected { background-color: %4; color: %5; }"
                 "QTabBar::tab:hover:!selected { background-color: %6; }")
             .arg(RAHMEN.name(), ZEBRA.name(), TEXT.name(), AKZENT.name(), AKZENT_TEXT.name(),
                  DEAKTIV_HINTERGRUND.name());

    // Gruppen, Statusleiste, Tooltips, Fortschrittsbalken
    s += QString("QGroupBox { border: 1px solid %1; margin-top: 14px; padding-top: 6px; }"
                 "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }")
             .arg(RAHMEN.name());
    s += QString("QStatusBar { background-color: %1; color: %2; }"
                 "QStatusBar QLabel { background-color: %1; color: %2; }")
             .arg(ZEBRA.name(), TEXT.name());
    s += QString("QToolTip { background-color: %1; color: %2; border: 1px solid %2; padding: 4px; }")
             .arg(TOOLTIP_HINTERGRUND.name(), TOOLTIP_TEXT.name());
    s += QString("QProgressBar { border: 1px solid %1; background-color: %2; color: %3; text-align: center; }"
                 "QProgressBar::chunk { background-color: %4; }")
             .arg(RAHMEN.name(), HINTERGRUND.name(), TEXT.name(), BALKEN_V.name());

    // Meldungsfelder: Eigenschaft "meldung" = ok, warn, fehler oder info
    const QString rahmenForm = "border: 1px solid %2; border-radius: 3px; padding: 6px; background-color: %1; color: %2;";
    s += "QLabel[meldung=\"ok\"] { " + rahmenForm.arg(OK_HINTERGRUND.name(), OK_TEXT.name()) + " }";
    s += "QLabel[meldung=\"warn\"] { " + rahmenForm.arg(WARN_HINTERGRUND.name(), WARN_TEXT.name()) + " }";
    s += "QLabel[meldung=\"fehler\"] { " + rahmenForm.arg(FEHLER_HINTERGRUND.name(), FEHLER_TEXT.name()) + " }";
    s += "QLabel[meldung=\"info\"] { " + rahmenForm.arg(INFO_HINTERGRUND.name(), INFO_TEXT.name()) + " }";

    // Hinweistexte unter Eingabefeldern
    s += QString("QLabel[hinweis=\"true\"] { color: %1; }").arg(HINWEIS_TEXT.name());

    // Kennzahlbalken
    s += QString("QProgressBar#pb_v::chunk { background-color: %1; }"
                 "QProgressBar#pb_l::chunk { background-color: %2; }"
                 "QProgressBar#pb_q::chunk { background-color: %3; }"
                 "QProgressBar#pb_oee::chunk { background-color: %4; }")
             .arg(BALKEN_V.name(), BALKEN_L.name(), BALKEN_Q.name(), BALKEN_OEE.name());

    // Kontrollkaestchen, Scrollleisten, Schieberegler und Splitter
    s += QString("QCheckBox { color: %1; spacing: 6px; }"
                 "QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid %2; background-color: %3; }"
                 "QCheckBox::indicator:checked { background-color: %4; border: 1px solid %1; }"
                 "QCheckBox:disabled { color: %5; }")
             .arg(TEXT.name(), RAHMEN.name(), HINTERGRUND.name(), AKZENT.name(), DEAKTIV_TEXT.name());

    s += QString("QScrollBar:vertical { background: %1; width: 14px; border: 1px solid %2; }"
                 "QScrollBar::handle:vertical { background: %2; min-height: 24px; }"
                 "QScrollBar::handle:vertical:hover { background: %3; }"
                 "QScrollBar:horizontal { background: %1; height: 14px; border: 1px solid %2; }"
                 "QScrollBar::handle:horizontal { background: %2; min-width: 24px; }"
                 "QScrollBar::handle:horizontal:hover { background: %3; }")
             .arg(ZEBRA.name(), RAHMEN.name(), TEXT.name());

    s += QString("QSlider::groove:horizontal { height: 6px; background: %1; border: 1px solid %2; border-radius: 3px; }"
                 "QSlider::handle:horizontal { background: %3; border: 1px solid %4; width: 14px; margin: -5px 0; border-radius: 7px; }"
                 "QSlider::handle:horizontal:disabled { background: %1; border: 1px solid %2; }")
             .arg(DEAKTIV_HINTERGRUND.name(), RAHMEN.name(), AKZENT.name(), TEXT.name());

    s += QString("QSplitter::handle { background-color: %1; }").arg(ZEBRA.name());
    return s;
}

QColor zustandFarbe(ZustandTyp typ)
{
    switch (typ) {
    case ZustandTyp::Produktion: return PRODUKTION;
    case ZustandTyp::Ruesten: return RUESTEN;
    case ZustandTyp::Stoerung: return STOERUNG;
    case ZustandTyp::Materialmangel: return MATERIALMANGEL;
    case ZustandTyp::Rueckstau: return RUECKSTAU;
    case ZustandTyp::Wartung: return WARTUNG;
    case ZustandTyp::Pause: return PAUSE;
    default: return UNBEKANNT;
    }
}

} // namespace Farben
