#ifndef DATENVERWALTUNG_H
#define DATENVERWALTUNG_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QDate>
#include <memory>
#include "model/datentypen.h"
#include "core/protokollcache.h"
#include "logic/oeeberechnung.h"

class QThread;

// Eine Zeile einer CSV-Datei (oder SQLite-Tabelle) mit ihrer Zeilennummer
struct TabellenZeile {
    int nummer = 0;
    QStringList felder;
};

// Haelt alle Stammdaten (CSV oder SQLite) und verwaltet die XML-Protokolle.
// Beim Laden eines Datensatzes wird alles automatisch geprueft. Gefundene
// Auffaelligkeiten werden als Problem (Flag) an den betroffenen Objekten
// gespeichert und in einer Gesamtliste gesammelt.
class Datenverwaltung : public QObject {
    Q_OBJECT

public:
    explicit Datenverwaltung(QObject* parent = nullptr);
    ~Datenverwaltung() override;

    // Sucht den Ordner RoboCell_Daten relativ zum Programm
    static QString standardOrdner();

    // Laedt und prueft einen kompletten Datensatz (synchron oder im Hintergrund)
    bool datensatzLaden(const QString& ordner);
    bool datensatzLadenHintergrund(const QString& ordner);
    bool pruefungLaeuft() const { return m_pruefungLaeuft; }
    void wartenAufPruefung();
    bool datensatzAusSqliteLaden(const QString& sqlitePfad);
    bool inSqliteSpeichern(const QString& sqlitePfad, QString* fehler = nullptr) const;
    bool csvExportieren(const QString& zielOrdner, QString* fehler = nullptr) const;

    const QString& ordner() const { return m_ordner; }

    // Stammdaten
    const QList<Linie>& linien() const { return m_linien; }
    const QList<Anlage>& anlagen() const { return m_anlagen; }
    const QList<Produkt>& produkte() const { return m_produkte; }
    const QList<Stoercode>& stoercodes() const { return m_stoercodes; }
    const QList<Wartung>& wartungen() const { return m_wartungen; }
    const Linie* findeLinie(const QString& linieId) const;
    const Anlage* findeAnlage(const QString& anlageId) const;
    const Stoercode* findeStoercode(const QString& code) const;
    QList<Wartung> wartungenVonAnlage(const QString& anlageId) const;

    // Aenderungen
    bool anlageAendern(const Anlage& neu);
    void wartungHinzufuegen(const Wartung& wartung);
    QString naechsteWartungsId() const;

    // Flags (automatisch erkannte Probleme)
    const QList<Problem>& alleProbleme() const { return m_probleme; }
    int anzahlProbleme(Schweregrad grad) const;
    int anzahlProtokollTagePruefung() const { return m_geprueft; }
    int anlagenProblemAnzahl(const QString& anlageId, Schweregrad grad) const;
    QList<Problem> anlagenProbleme(const QString& anlageId) const;   // Stammdaten + Protokolle
    QList<Problem> tagesProbleme(const QString& anlageId, const QDate& datum) const;

    // Protokolle
    QList<QDate> verfuegbareTage(const QString& anlageId) const;
    QList<QDate> alleTage() const;
    QString protokollPfad(const QString& anlageId, const QDate& datum) const;
    std::shared_ptr<Schichtprotokoll> protokoll(const QString& anlageId, const QDate& datum);

    // Auswertungen (aus den beim Laden berechneten Tageskennzahlen)
    OeeKennzahlen tageskennzahlen(const QString& anlageId, const QDate& datum) const;
    QList<OeeKennzahlen> alleTageskennzahlen(const QString& anlageId) const;
    Trendbericht trendbericht(const QString& anlageId);
    QList<EngpassEintrag> engpass(const QString& linieId, const QDate& datum) const;
    QList<ParetoEintrag> pareto(const QString& linieId) const;

    // Cache
    void setCacheGroesse(int groesse) { m_cache.setKapazitaet(groesse); }
    void cacheLeeren() { m_cache.leeren(); }
    int cacheGroesse() const { return m_cache.groesse(); }
    int cacheTreffer() const { return m_cache.treffer(); }
    int cacheFehlversuche() const { return m_cache.fehlversuche(); }

signals:
    void pruefFortschritt(int aktuell, int gesamt);
    void pruefungAbgeschlossen();
    void datensatzGeladen();
    void anlageGeaendert(const QString& anlageId);
    void wartungHinzugefuegt(const QString& anlageId);

private:
    // Hilfsfunktionen zum Lesen
    static QStringList csvZeileTeilen(const QString& zeile);
    QList<TabellenZeile> csvLesen(const QString& dateiName, int erwarteteSpalten);
    QList<TabellenZeile> sqliteLesen(const QString& verbindung, const QString& tabelle,
                                     const QStringList& spalten);

    // Verarbeitung der Tabellen (gemeinsam fuer CSV und SQLite)
    void leereAlles();
    void linienVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle);
    void anlagenVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle);
    void produkteVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle);
    void stoercodesVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle);
    void wartungenVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle);
    void indexLaden();
    void indexVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle,
                          const QString& pfadPraefix);
    void indexEintragPruefen(const QString& schl, ProtokollIndexEintrag* e);
    void protokolleAutomatischPruefen();
    void anlagenWertePruefen(Anlage* anlage, int zeile, const QString& quelle);
    void positionenPruefen();
    void problemeSammeln();
    void kennzahlenNeuBerechnen(const QString& anlageId);
    bool stammUndIndexAusOrdnerLaden(const QString& ordner);
    void hintergrundPruefungStoppen();

    bool zahlLesen(const QString& text, const QString& feld, double* wert, QList<Problem>* probleme,
                   const QString& quelle, int zeile, const QString& id) const;
    void dateiProblem(Schweregrad grad, const QString& code, const QString& quelle, int zeile,
                      const QString& id, const QStringList& werte = QStringList());

    static QString schluessel(const QString& anlageId, const QDate& datum);

    QString m_ordner;
    QList<Linie> m_linien;
    QList<Anlage> m_anlagen;
    QList<Produkt> m_produkte;
    QList<Stoercode> m_stoercodes;
    QList<Wartung> m_wartungen;

    QSet<QString> m_linienIds;
    QSet<QString> m_anlagenIds;
    QSet<QString> m_produktIds;
    QSet<QString> m_stoercodeIds;
    QMap<QString, QString> m_produktLinie;

    QMap<QString, ProtokollIndexEintrag> m_index;
    QMap<QString, QList<QDate>> m_tageJeAnlage;
    QMap<QString, OeeKennzahlen> m_kennzahlen;
    QMap<QString, Trendbericht> m_trends;

    QList<Problem> m_dateiProbleme;   // Probleme, die zu keinem Objekt gehoeren
    QList<Problem> m_probleme;        // alle Probleme (Gesamtliste)
    int m_geprueft = 0;

    QList<QThread*> m_ladeThreads;
    bool m_pruefungLaeuft = false;
    int m_ladeLaufId = 0;
    int m_offeneThreads = 0;
    int m_fertigGeprueft = 0;

    ProtokollCache m_cache;
};

#endif // DATENVERWALTUNG_H
