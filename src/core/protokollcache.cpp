#include "protokollcache.h"

ProtokollCache::ProtokollCache(int kapazitaet)
    : m_kapazitaet(kapazitaet > 0 ? kapazitaet : 1)
{
}

void ProtokollCache::setKapazitaet(int kapazitaet)
{
    m_kapazitaet = kapazitaet > 0 ? kapazitaet : 1;
    aeltesteEntfernen();
}

std::shared_ptr<Schichtprotokoll> ProtokollCache::holen(const QString& schluessel)
{
    if (!m_daten.contains(schluessel)) {
        m_fehlversuche++;
        return nullptr;
    }
    // Eintrag nach vorne schieben, weil er gerade benutzt wurde
    m_reihenfolge.removeAll(schluessel);
    m_reihenfolge.prepend(schluessel);
    m_treffer++;
    return m_daten.value(schluessel);
}

void ProtokollCache::ablegen(const QString& schluessel, std::shared_ptr<Schichtprotokoll> protokoll)
{
    m_reihenfolge.removeAll(schluessel);
    m_reihenfolge.prepend(schluessel);
    m_daten.insert(schluessel, protokoll);
    aeltesteEntfernen();
}

void ProtokollCache::aeltesteEntfernen()
{
    while (m_reihenfolge.size() > m_kapazitaet) {
        const QString aeltester = m_reihenfolge.takeLast();
        m_daten.remove(aeltester);
    }
}

void ProtokollCache::leeren()
{
    m_reihenfolge.clear();
    m_daten.clear();
    m_treffer = 0;
    m_fehlversuche = 0;
}
