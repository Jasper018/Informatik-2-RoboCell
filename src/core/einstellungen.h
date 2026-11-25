#ifndef EINSTELLUNGEN_H
#define EINSTELLUNGEN_H

#include <QByteArray>
#include <QString>

// Dauerhaft gespeicherte Einstellungen (QSettings, unter Windows in der Registry)
namespace Einstellungen {

QString sprache();                       // "de" oder "en"
void setSprache(const QString& code);

QString datenordner();                   // leer = Standardordner suchen
void setDatenordner(const QString& ordner);

int schriftgroesse();                    // in Punkt
void setSchriftgroesse(int punkt);

int cacheGroesse();                      // Anzahl Protokolle im LRU-Cache
void setCacheGroesse(int anzahl);

bool dunkelmodus();
void setDunkelmodus(bool aktiv);

QByteArray fensterGeometrie();
void setFensterGeometrie(const QByteArray& daten);

} // namespace Einstellungen

#endif // EINSTELLUNGEN_H
