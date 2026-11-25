#ifndef FARBEN_H
#define FARBEN_H

#include <QColor>
#include <QList>
#include <QString>
#include "model/datentypen.h"

// Zentrale Farbpalette der Anwendung.
// Jede Textfarbe und jede Diagrammfarbe erreicht gegen ihren Hintergrund ein
// Kontrastverhaeltnis von mindestens 8:1 (Formel der WCAG 2.x). Die Paare
// werden in alleFarbpaare() aufgelistet und im Test geprueft.
namespace Farben {

// Grundfarben
inline const QColor HINTERGRUND("#ffffff");
inline const QColor TEXT("#111111");
inline const QColor ZEBRA("#f0f0f0");             // jede zweite Tabellenzeile
inline const QColor HINWEIS_TEXT("#3d3d3d");      // Nebentexte, Erklaerungen
inline const QColor RAHMEN("#4a4a4a");
inline const QColor AKZENT("#0b3d91");            // Buttons, Auswahl
inline const QColor AKZENT_HOVER("#082a66");
inline const QColor AKZENT_TEXT("#ffffff");
inline const QColor KOPF("#1f2937");              // Menueleiste, Tabellenkopf
inline const QColor KOPF_TEXT("#ffffff");
inline const QColor DEAKTIV_HINTERGRUND("#e6e6e6");
inline const QColor DEAKTIV_TEXT("#3a3a3a");
inline const QColor TOOLTIP_HINTERGRUND("#1a1a1a");
inline const QColor TOOLTIP_TEXT("#ffffff");

// Meldungen (Hintergrund / Text)
inline const QColor OK_HINTERGRUND("#d4f0d4");
inline const QColor OK_TEXT("#052e16");
inline const QColor WARN_HINTERGRUND("#ffe8a6");
inline const QColor WARN_TEXT("#3b2300");
inline const QColor FEHLER_HINTERGRUND("#ffd0d0");
inline const QColor FEHLER_TEXT("#4a0404");
inline const QColor INFO_HINTERGRUND("#d6e8ff");
inline const QColor INFO_TEXT("#0a2540");

// Fortschrittsbalken der Kennzahlen (heller Balken, dunkler Text)
inline const QColor BALKEN_V("#a8c8ff");
inline const QColor BALKEN_L("#ffd27a");
inline const QColor BALKEN_Q("#9ee59e");
inline const QColor BALKEN_OEE("#c4c4c4");

// Zustandsfarben (dunkel, damit sie auf weissem Grund 8:1 erreichen)
inline const QColor PRODUKTION("#14532d");
inline const QColor RUESTEN("#1e3a8a");
inline const QColor STOERUNG("#7f1d1d");
inline const QColor MATERIALMANGEL("#78350f");
inline const QColor RUECKSTAU("#581c87");
inline const QColor WARTUNG("#0b4a46");
inline const QColor PAUSE("#374151");
inline const QColor UNBEKANNT("#000000");

// Linien in den Diagrammen
inline const QColor LINIE_V("#0b3d91");
inline const QColor LINIE_L("#78350f");
inline const QColor LINIE_Q("#14532d");
inline const QColor LINIE_OEE("#111111");
inline const QColor LINIE_AUSSCHUSS("#7f1d1d");

// Ein Farbpaar (Vordergrund auf Hintergrund) zur Kontrastpruefung
struct Farbpaar {
    QString name;
    QColor vordergrund;
    QColor hintergrund;
};

// Relative Helligkeit nach WCAG
double helligkeit(const QColor& farbe);

// Kontrastverhaeltnis (1.0 bis 21.0)
double kontrast(const QColor& a, const QColor& b);

// Alle Text- und Diagrammfarben mit ihrem jeweiligen Hintergrund
QList<Farbpaar> alleFarbpaare();

// Farbe eines Betriebszustands aus der zentralen Palette
QColor zustandFarbe(ZustandTyp typ);

// Gemeinsames Style Sheet der Anwendung
QString styleSheet();

} // namespace Farben

#endif // FARBEN_H
