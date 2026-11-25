#ifndef PROTOKOLLCACHE_H
#define PROTOKOLLCACHE_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <memory>
#include "model/datentypen.h"

// Einfacher LRU-Cache (Least Recently Used) fuer geladene Schichtprotokolle.
// Vorne in der Liste steht das zuletzt benutzte Protokoll, hinten das aelteste.
class ProtokollCache {
public:
    explicit ProtokollCache(int kapazitaet = 60);

    void setKapazitaet(int kapazitaet);
    int kapazitaet() const { return m_kapazitaet; }

    // Liefert nullptr, wenn das Protokoll nicht im Cache liegt
    std::shared_ptr<Schichtprotokoll> holen(const QString& schluessel);
    void ablegen(const QString& schluessel, std::shared_ptr<Schichtprotokoll> protokoll);
    void leeren();

    int groesse() const { return m_daten.size(); }
    int treffer() const { return m_treffer; }
    int fehlversuche() const { return m_fehlversuche; }

private:
    void aeltesteEntfernen();

    int m_kapazitaet;
    int m_treffer = 0;
    int m_fehlversuche = 0;
    QStringList m_reihenfolge;
    QMap<QString, std::shared_ptr<Schichtprotokoll>> m_daten;
};

#endif // PROTOKOLLCACHE_H
