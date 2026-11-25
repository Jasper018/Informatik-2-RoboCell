#include <QApplication>
#include <QFont>
#include <QPalette>
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

    // Hoher Kontrast (>= 8:1) fuer die gesamte Anwendung, inkl. Platzhaltertext
    QPalette pal = app.palette();
    pal.setColor(QPalette::Window, Farben::HINTERGRUND);
    pal.setColor(QPalette::WindowText, Farben::TEXT);
    pal.setColor(QPalette::Base, Farben::HINTERGRUND);
    pal.setColor(QPalette::AlternateBase, Farben::ZEBRA);
    pal.setColor(QPalette::Text, Farben::TEXT);
    pal.setColor(QPalette::PlaceholderText, Farben::HINWEIS_TEXT);
    pal.setColor(QPalette::Button, Farben::AKZENT);
    pal.setColor(QPalette::ButtonText, Farben::AKZENT_TEXT);
    pal.setColor(QPalette::Highlight, Farben::AKZENT);
    pal.setColor(QPalette::HighlightedText, Farben::AKZENT_TEXT);
    app.setPalette(pal);
    app.setStyleSheet(Farben::styleSheet());

    QFont schrift = app.font();
    schrift.setPointSize(Einstellungen::schriftgroesse());
    app.setFont(schrift);

    Sprache::setzen(Einstellungen::sprache());

    // Anmeldung mit den im Hintergrund gespeicherten Profilen
    Benutzerverwaltung benutzer;
    AnmeldeDialog anmeldung(&benutzer);
    if (anmeldung.exec() != QDialog::Accepted) {
        return 0;
    }

    // Datensatz laden (dabei werden CSV-Dateien und XML-Protokolle automatisch geprueft)
    Datenverwaltung daten;
    daten.setCacheGroesse(Einstellungen::cacheGroesse());
    const QString ordner = Einstellungen::datenordner().isEmpty()
        ? Datenverwaltung::standardOrdner()
        : Einstellungen::datenordner();
    daten.datensatzLaden(ordner);

    MainWindow fenster(&daten, &benutzer);
    fenster.show();
    return app.exec();
}
