#include <QApplication>
#include <QFont>
#include <QStyleFactory>
#include "core/benutzerverwaltung.h"
#include "core/einstellungen.h"
#include "core/farben.h"
#include "core/sprache.h"
#include "io/datenverwaltung.h"
#include "ui/anmeldedialog.h"
#include "ui/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("RoboCell");
    app.setOrganizationName("DHBW_TMT25");
    app.setStyle(QStyleFactory::create("Fusion"));

    // Hoher Kontrast (>= 8:1) im gewaehlten Modus (Hell oder Dunkel)
    Farben::setDunkelmodus(Einstellungen::dunkelmodus());
    app.setPalette(Farben::anwendungsPalette());
    app.setStyleSheet(Farben::styleSheet());

    QFont schrift = app.font();
    schrift.setPointSize(Einstellungen::schriftgroesse());
    app.setFont(schrift);

    Sprache::setzen(Einstellungen::sprache());

    // Datensatz sofort waehrend der Anmeldung im Hintergrund laden und pruefen,
    // damit nach der Anmeldung keine Wartezeit entsteht.
    Datenverwaltung daten;
    daten.setCacheGroesse(Einstellungen::cacheGroesse());
    const QString ordner = Einstellungen::datenordner().isEmpty()
        ? Datenverwaltung::standardOrdner()
        : Einstellungen::datenordner();
    daten.datensatzLadenHintergrund(ordner);

    // Anmeldung mit den im Hintergrund gespeicherten Profilen
    Benutzerverwaltung benutzer;
    AnmeldeDialog anmeldung(&benutzer);
    if (anmeldung.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow fenster(&daten, &benutzer);
    fenster.show();
    return app.exec();
}
