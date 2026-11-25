#ifndef FENSTERHILFE_H
#define FENSTERHILFE_H

#include <QByteArray>

class QWidget;

// Hilfsfunktionen, damit Fenster auf jedem Bildschirm vollstaendig sichtbar bleiben
namespace Fensterhilfe {

// Begrenzt die Groesse des Fensters auf faktor * verfuegbare Bildschirmflaeche
// und zentriert es darin.
void fensterAnBildschirmAnpassen(QWidget* fenster, int wunschBreite, int wunschHoehe,
                                 double faktor = 0.90);

// Stellt eine gespeicherte Fenstergeometrie wieder her, prueft aber gegen
// den aktuellen Bildschirm, sodass nichts ausserhalb liegt oder zu gross wird.
void geometrieSicherWiederherstellen(QWidget* fenster, const QByteArray& daten,
                                      int wunschBreite, int wunschHoehe);

} // namespace Fensterhilfe

#endif // FENSTERHILFE_H
