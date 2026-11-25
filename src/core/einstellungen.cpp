#include "einstellungen.h"
#include <QSettings>

static QSettings& einstellungen()
{
    static QSettings s("DHBW_TMT25", "RoboCell");
    return s;
}

QString Einstellungen::sprache()
{
    return einstellungen().value("sprache", "de").toString();
}

void Einstellungen::setSprache(const QString& code)
{
    einstellungen().setValue("sprache", code);
}

QString Einstellungen::datenordner()
{
    return einstellungen().value("datenordner", QString()).toString();
}

void Einstellungen::setDatenordner(const QString& ordner)
{
    einstellungen().setValue("datenordner", ordner);
}

int Einstellungen::schriftgroesse()
{
    return einstellungen().value("schriftgroesse", 10).toInt();
}

void Einstellungen::setSchriftgroesse(int punkt)
{
    einstellungen().setValue("schriftgroesse", punkt);
}

int Einstellungen::cacheGroesse()
{
    return einstellungen().value("cacheGroesse", 60).toInt();
}

void Einstellungen::setCacheGroesse(int anzahl)
{
    einstellungen().setValue("cacheGroesse", anzahl);
}

QByteArray Einstellungen::fensterGeometrie()
{
    return einstellungen().value("fensterGeometrie").toByteArray();
}

void Einstellungen::setFensterGeometrie(const QByteArray& daten)
{
    einstellungen().setValue("fensterGeometrie", daten);
}
