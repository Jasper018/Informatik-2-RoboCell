#pragma once

#include "model/Entities.h"
#include "core/LruCache.h"
#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QDate>
#include <memory>

namespace RoboCell {

/**
 * @brief Zentrales Repository für alle Stammdaten (CSV & SQLite) und Lazy-Loading der XML-Schichtprotokolle.
 */
class DataRepository : public QObject {
    Q_OBJECT
public:
    explicit DataRepository(QObject* parent = nullptr);

    /**
     * @brief Sucht automatisch den relativen Pfad zum Ordner RoboCell_Daten.
     */
    static QString findDefaultDataDirectory();

    /**
     * @brief Lädt alle globalen CSV-Dateien und den Master-Protokoll-Index aus dataDir.
     */
    bool loadAllFromDirectory(const QString& dataDir);

    /**
     * @brief Lädt die Stammdaten alternativ aus einer SQLite-Datenbank (Erweiterung 10).
     */
    bool loadFromSqlite(const QString& sqliteFilePath);

    /**
     * @brief Speichert die aktuellen Stammdaten in eine SQLite-Datenbank (Erweiterung 10).
     */
    bool saveToSqlite(const QString& sqliteFilePath, QString* errorOut = nullptr) const;

    /**
     * @brief Exportiert die (bearbeiteten) Stammdaten in neue CSV-Dateien (anlagen_out.csv, wartungen_out.csv, etc.).
     */
    bool exportModifiedCsv(const QString& targetDir, QString* errorOut = nullptr) const;

    const QString& dataDirectory() const { return m_dataDir; }

    // Getter für Stammdaten
    const QList<Linie>& linien() const { return m_linien; }
    const QList<Anlage>& anlagen() const { return m_anlagen; }
    QList<Anlage>& anlagenMutable() { return m_anlagen; }
    const QList<Produkt>& produkte() const { return m_produkte; }
    const QList<Stoercode>& stoercodes() const { return m_stoercodes; }
    const QList<Wartung>& wartungen() const { return m_wartungen; }
    const QList<ValidationIssue>& validationIssues() const { return m_issues; }

    const Linie* findLinie(const QString& linieId) const;
    const Anlage* findAnlage(const QString& anlageId) const;
    Anlage* findAnlageMutable(const QString& anlageId);
    const Produkt* findProdukt(const QString& produktId) const;
    const Stoercode* findStoercode(const QString& code) const;
    QList<Wartung> wartungenForAnlage(const QString& anlageId) const;

    // Bearbeitung von Stammdaten & Wartungen
    bool updateAnlage(const Anlage& updated);
    void addWartung(const Wartung& newWartung);

    // Protokoll-Index & Lazy-Loading (Mindestanforderung 5)
    QList<QDate> availableDatesForAnlage(const QString& anlageId) const;
    QList<QDate> allAvailableDates() const;
    QString relativeXmlPath(const QString& anlageId, const QDate& datum) const;

    /**
     * @brief Lädt das Schichtprotokoll für (anlageId, datum) lazy (mit LRU-Cache).
     */
    std::shared_ptr<Schichtprotokoll> getProtokollLazy(const QString& anlageId, const QDate& datum);

    /**
     * @brief Lädt alle verfügbaren Tage einer Anlage (für 14-Tage-Trendanalyse).
     */
    QList<std::shared_ptr<Schichtprotokoll>> loadAllProtokolleForAnlage(const QString& anlageId);

    // Cache-Statistiken (Erweiterung 5)
    size_t cacheSize() const { return m_protokollCache.size(); }
    size_t cacheHits() const { return m_protokollCache.hits(); }
    size_t cacheMisses() const { return m_protokollCache.misses(); }
    void clearCache() { m_protokollCache.clear(); }

    /**
     * @brief Parst eine CSV-Zeile unter Berücksichtigung von Anführungszeichen (z. B. "33,2").
     */
    static QStringList splitCsvLine(const QString& line);

signals:
    void dataReloaded();
    void anlageUpdated(const QString& anlageId);
    void wartungAdded(const QString& anlageId);

private:
    void loadLinienCsv(const QString& filePath);
    void loadAnlagenCsv(const QString& filePath);
    void loadProdukteCsv(const QString& filePath);
    void loadStoercodesCsv(const QString& filePath);
    void loadWartungenCsv(const QString& filePath);
    void loadMasterIndexCsv(const QString& filePath);

    void addIssue(ValidationIssue::Severity sev, const QString& file, int line,
                  const QString& id, const QString& de, const QString& en);

    QString m_dataDir;
    QList<Linie> m_linien;
    QList<Anlage> m_anlagen;
    QList<Produkt> m_produkte;
    QList<Stoercode> m_stoercodes;
    QList<Wartung> m_wartungen;
    QList<ValidationIssue> m_issues;

    // Schnelle Lookups
    QSet<QString> m_linienIds;
    QSet<QString> m_anlagenIds;
    QSet<QString> m_produktIds;
    QSet<QString> m_stoercodeIds;

    // Schlüssel: "A001|2026-09-14" -> ProtokollIndexEntry
    QMap<QString, ProtokollIndexEntry> m_protokollIndex;
    QMap<QString, QList<QDate>> m_datesByAnlage;

    mutable LruCache<QString, std::shared_ptr<Schichtprotokoll>> m_protokollCache{120};
};

} // namespace RoboCell
