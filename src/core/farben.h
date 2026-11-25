#ifndef FARBEN_H
#define FARBEN_H

#include <QColor>
#include <QList>
#include <QPalette>
#include <QString>
#include "model/datentypen.h"

// Zentrale Farbpalette der Anwendung (Heller Modus und Dunkler Modus).
// Jede Textfarbe und jede Diagrammfarbe erreicht gegen ihren Hintergrund ein
// Kontrastverhaeltnis von mindestens 8:1 (Formel der WCAG 2.x). Die Paare
// werden in alleFarbpaare() aufgelistet und im Test geprueft.
namespace Farben {

bool dunkelmodus();
void setDunkelmodus(bool aktiv);

// Grundfarben je nach aktivem Modus
QColor hintergrund();
QColor text();
QColor zebra();              // jede zweite Tabellenzeile
QColor hinweisText();        // Nebentexte, Erklaerungen
QColor rahmen();
QColor akzent();             // Buttons, Auswahl
QColor akzentHover();
QColor akzentText();
QColor kopf();               // Menueleiste, Tabellenkopf
QColor kopfText();
QColor deaktivHintergrund();
QColor deaktivText();
QColor tooltipHintergrund();
QColor tooltipText();

// Meldungen (Hintergrund / Text)
QColor okHintergrund();
QColor okText();
QColor warnHintergrund();
QColor warnText();
QColor fehlerHintergrund();
QColor fehlerText();

// Fortschrittsbalken der Kennzahlen (Balken / Lesetext ist text())
QColor balkenV();
QColor balkenL();
QColor balkenQ();
QColor balkenOee();

// Zustandsfarben und passender Kontrasttext darauf
QColor zustandFarbe(ZustandTyp typ);
QColor zustandTextFarbe();

// Linien in den Diagrammen
QColor linieV();
QColor linieL();
QColor linieQ();
QColor linieOee();
QColor linieAusschuss();

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
QList<Farbpaar> alleFarbpaare(bool dunkel);
QList<Farbpaar> alleFarbpaare();

// Gemeinsames Style Sheet und Palette der Anwendung
QPalette anwendungsPalette();
QString styleSheet();

} // namespace Farben

#endif // FARBEN_H
