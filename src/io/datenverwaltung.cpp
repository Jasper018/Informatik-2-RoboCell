#include "datenverwaltung.h"
#include "protokollleser.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>
#include <algorithm>

// Spalten der Tabellen (Reihenfolge wie in den CSV-Dateien)
static const QStringList SPALTEN_LINIEN = {"linie_id", "bezeichnung", "halle", "schichtmodell"};
static const QStringList SPALTEN_ANLAGEN = {"anlage_id", "linie_id", "position", "bezeichnung", "typ",
                                            "hersteller", "modell", "achsen", "traglast_kg", "baujahr",
                                            "soll_taktzeit_s", "status"};
static const QStringList SPALTEN_PRODUKTE = {"produkt_id", "linie_id", "bezeichnung"};
static const QStringList SPALTEN_STOERCODES = {"code", "kategorie", "beschreibung"};
static const QStringList SPALTEN_WARTUNGEN = {"wartung_id", "anlage_id", "datum", "art", "dauer_min",
                                              "geplant", "bemerkung"};
static const QStringList SPALTEN_INDEX = {"anlage_id", "datum", "pfad", "anzahl_zustaende"};

static const QStringList KATEGORIEN = {"mechanik", "elektrik", "steuerung", "peripherie", "material", "qualitaet"};

static const int BAUJAHR_MIN = 1980;

Datenverwaltung::Datenverwaltung(QObject* parent)
    : QObject(parent), m_cache(60)
{
}

QString Datenverwaltung::standardOrdner()
{
    const QStringList kandidaten = {"RoboCell_Daten", "data", "../RoboCell_Daten",
                                    "../../RoboCell_Daten", "../../../RoboCell_Daten"};
    const QString programmOrdner = QCoreApplication::applicationDirPath();
    for (const QString& relativ : kandidaten) {
        QDir d1(QDir::current().filePath(relativ));
        if (QFile::exists(d1.filePath("anlagen.csv"))) return d1.absolutePath();
        QDir d2(QDir(programmOrdner).filePath(relativ));
        if (QFile::exists(d2.filePath("anlagen.csv"))) return d2.absolutePath();
    }
    return QDir("RoboCell_Daten").absolutePath();
}

QString Datenverwaltung::schluessel(const QString& anlageId, const QDate& datum)
{
    return anlageId + "|" + datum.toString("yyyy-MM-dd");
}

// ---------------------------------------------------------------------------
// Hilfsfunktionen zum Lesen
// ---------------------------------------------------------------------------
QStringList Datenverwaltung::csvZeileTeilen(const QString& zeile)
{
    // Trennzeichen ist das Komma. Felder in Anfuehrungszeichen duerfen Kommas enthalten.
    QStringList felder;
    QString aktuell;
    bool inAnfuehrung = false;

    for (int i = 0; i < zeile.size(); i++) {
        const QChar c = zeile.at(i);
        if (c == '"') {
            if (inAnfuehrung && i + 1 < zeile.size() && zeile.at(i + 1) == '"') {
                aktuell.append('"');
                i++;
            } else {
                inAnfuehrung = !inAnfuehrung;
            }
        } else if (c == ',' && !inAnfuehrung) {
            felder.append(aktuell.trimmed());
            aktuell.clear();
        } else {
            aktuell.append(c);
        }
    }
    felder.append(aktuell.trimmed());
    return felder;
}

void Datenverwaltung::dateiProblem(Schweregrad grad, const QString& code, const QString& quelle,
                                   int zeile, const QString& id, const QStringList& werte)
{
    m_dateiProbleme.append(neuesProblem(grad, code, quelle, zeile, id, werte));
}

QList<TabellenZeile> Datenverwaltung::csvLesen(const QString& dateiName, int erwarteteSpalten)
{
    QList<TabellenZeile> zeilen;
    QFile datei(QDir(m_ordner).filePath(dateiName));
    if (!datei.open(QIODevice::ReadOnly | QIODevice::Text)) {
        dateiProblem(Schweregrad::Fehler, "DATEI_OEFFNEN", dateiName, 0, "-", {dateiName});
        return zeilen;
    }

    QTextStream in(&datei);
    in.setEncoding(QStringConverter::Utf8);
    int nummer = 0;
    while (!in.atEnd()) {
        const QString text = in.readLine().trimmed();
        nummer++;
        if (text.isEmpty()) continue;

        TabellenZeile z;
        z.nummer = nummer;
        z.felder = csvZeileTeilen(text);
        if (nummer == 1) {
            // Kopfzeile nur pruefen, nicht weiterverarbeiten
            if (z.felder.size() != erwarteteSpalten) {
                dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", dateiName, 1, "-",
                             {QString::number(z.felder.size()), QString::number(erwarteteSpalten)});
            }
            continue;
        }
        zeilen.append(z);
    }
    return zeilen;
}

QList<TabellenZeile> Datenverwaltung::sqliteLesen(const QString& verbindung, const QString& tabelle,
                                                  const QStringList& spalten)
{
    QList<TabellenZeile> zeilen;
    QSqlDatabase db = QSqlDatabase::database(verbindung);
    QSqlQuery abfrage(db);
    if (!abfrage.exec("SELECT " + spalten.join(", ") + " FROM " + tabelle)) {
        dateiProblem(Schweregrad::Fehler, "DATEI_OEFFNEN", tabelle + " (SQLite)", 0, "-", {tabelle});
        return zeilen;
    }
    int nummer = 1;
    while (abfrage.next()) {
        nummer++;
        TabellenZeile z;
        z.nummer = nummer;
        for (int i = 0; i < spalten.size(); i++) {
            z.felder.append(abfrage.value(i).toString().trimmed());
        }
        zeilen.append(z);
    }
    return zeilen;
}

bool Datenverwaltung::zahlLesen(const QString& text, const QString& feld, double* wert,
                                QList<Problem>* probleme, const QString& quelle, int zeile,
                                const QString& id) const
{
    QString bereinigt = text.trimmed();
    if (bereinigt.contains(',') && !bereinigt.contains('.')) {
        probleme->append(neuesProblem(Schweregrad::Warnung, "DEZIMALKOMMA", quelle, zeile, id, {feld, text}));
        bereinigt.replace(',', '.');
    }
    bool ok = false;
    *wert = bereinigt.toDouble(&ok);
    if (!ok) {
        probleme->append(neuesProblem(Schweregrad::Fehler, "ZAHL_UNGUELTIG", quelle, zeile, id, {feld, text}));
    }
    return ok;
}

// ---------------------------------------------------------------------------
// Datensatz laden
// ---------------------------------------------------------------------------
void Datenverwaltung::leereAlles()
{
    m_linien.clear();
    m_anlagen.clear();
    m_produkte.clear();
    m_stoercodes.clear();
    m_wartungen.clear();
    m_linienIds.clear();
    m_anlagenIds.clear();
    m_produktIds.clear();
    m_stoercodeIds.clear();
    m_produktLinie.clear();
    m_index.clear();
    m_tageJeAnlage.clear();
    m_kennzahlen.clear();
    m_trends.clear();
    m_dateiProbleme.clear();
    m_probleme.clear();
    m_geprueft = 0;
    m_cache.leeren();
}

bool Datenverwaltung::datensatzLaden(const QString& ordner)
{
    leereAlles();
    m_ordner = ordner;

    if (!QDir(ordner).exists()) {
        dateiProblem(Schweregrad::Fehler, "DATEI_OEFFNEN", ordner, 0, "-", {ordner});
        problemeSammeln();
        emit datensatzGeladen();
        return false;
    }

    linienVerarbeiten(csvLesen("linien.csv", SPALTEN_LINIEN.size()), "linien.csv");
    anlagenVerarbeiten(csvLesen("anlagen.csv", SPALTEN_ANLAGEN.size()), "anlagen.csv");
    produkteVerarbeiten(csvLesen("produkte.csv", SPALTEN_PRODUKTE.size()), "produkte.csv");
    stoercodesVerarbeiten(csvLesen("stoercodes.csv", SPALTEN_STOERCODES.size()), "stoercodes.csv");
    wartungenVerarbeiten(csvLesen("wartungen.csv", SPALTEN_WARTUNGEN.size()), "wartungen.csv");
    positionenPruefen();

    indexLaden();
    protokolleAutomatischPruefen();
    problemeSammeln();
    emit datensatzGeladen();
    return !m_anlagen.isEmpty();
}

bool Datenverwaltung::datensatzAusSqliteLaden(const QString& sqlitePfad)
{
    leereAlles();
    m_ordner = QFileInfo(sqlitePfad).absolutePath();

    const QString verbindung = "RoboCellLesen";
    bool geoeffnet = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", verbindung);
        db.setDatabaseName(sqlitePfad);
        geoeffnet = QFile::exists(sqlitePfad) && db.open();
        if (geoeffnet) {
            linienVerarbeiten(sqliteLesen(verbindung, "linien", SPALTEN_LINIEN), "linien (SQLite)");
            anlagenVerarbeiten(sqliteLesen(verbindung, "anlagen", SPALTEN_ANLAGEN), "anlagen (SQLite)");
            produkteVerarbeiten(sqliteLesen(verbindung, "produkte", SPALTEN_PRODUKTE), "produkte (SQLite)");
            stoercodesVerarbeiten(sqliteLesen(verbindung, "stoercodes", SPALTEN_STOERCODES), "stoercodes (SQLite)");
            wartungenVerarbeiten(sqliteLesen(verbindung, "wartungen", SPALTEN_WARTUNGEN), "wartungen (SQLite)");
            db.close();
        } else {
            dateiProblem(Schweregrad::Fehler, "DATEI_OEFFNEN", sqlitePfad, 0, "-", {sqlitePfad});
        }
    }
    QSqlDatabase::removeDatabase(verbindung);

    if (geoeffnet) {
        positionenPruefen();
        indexLaden();
        protokolleAutomatischPruefen();
    }
    problemeSammeln();
    emit datensatzGeladen();
    return geoeffnet && !m_anlagen.isEmpty();
}

// ---------------------------------------------------------------------------
// Verarbeitung der Tabellen mit automatischer Pruefung
// ---------------------------------------------------------------------------
void Datenverwaltung::linienVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle)
{
    for (const TabellenZeile& z : zeilen) {
        if (z.felder.size() != SPALTEN_LINIEN.size()) {
            dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", quelle, z.nummer, "-",
                         {QString::number(z.felder.size()), QString::number(SPALTEN_LINIEN.size())});
            continue;
        }
        Linie l;
        l.linieId = z.felder[0];
        l.bezeichnung = z.felder[1];
        l.halle = z.felder[2];
        l.schichtmodell = z.felder[3];

        if (l.linieId.isEmpty()) {
            dateiProblem(Schweregrad::Fehler, "WERT_LEER", quelle, z.nummer, "-", {"linie_id"});
            continue;
        }
        if (m_linienIds.contains(l.linieId)) {
            dateiProblem(Schweregrad::Warnung, "ID_DOPPELT", quelle, z.nummer, l.linieId, {l.linieId});
            continue;
        }
        if (l.schichtmodell != "3-schicht" && l.schichtmodell != "2-schicht") {
            dateiProblem(Schweregrad::Warnung, "SCHICHTMODELL_UNBEKANNT", quelle, z.nummer,
                         l.linieId, {l.schichtmodell});
        }
        m_linienIds.insert(l.linieId);
        m_linien.append(l);
    }
}

void Datenverwaltung::anlagenWertePruefen(Anlage* a, int zeile, const QString& quelle)
{
    const QString id = a->anlageId;

    if (a->sollTaktzeitS <= 0.0) {
        a->probleme.append(neuesProblem(Schweregrad::Fehler, "TAKT_UNGUELTIG", quelle, zeile, id,
                                        {QString::number(a->sollTaktzeitS)}));
    }
    if (a->typ == AnlagenTyp::Unbekannt) {
        a->probleme.append(neuesProblem(Schweregrad::Fehler, "TYP_UNBEKANNT", quelle, zeile, id, {a->typRoh}));
    }
    if (a->status == AnlagenStatus::Unbekannt) {
        a->probleme.append(neuesProblem(Schweregrad::Fehler, "STATUS_UNBEKANNT", quelle, zeile, id, {a->statusRoh}));
    }
    const int aktuellesJahr = QDate::currentDate().year();
    if (a->baujahr < BAUJAHR_MIN || a->baujahr > aktuellesJahr) {
        a->probleme.append(neuesProblem(Schweregrad::Warnung, "BAUJAHR_UNPLAUSIBEL", quelle, zeile, id,
                                        {QString::number(a->baujahr), QString::number(BAUJAHR_MIN),
                                         QString::number(aktuellesJahr)}));
    }
    if (a->achsen < 0) {
        a->probleme.append(neuesProblem(Schweregrad::Fehler, "WERT_NEGATIV", quelle, zeile, id,
                                        {"achsen", QString::number(a->achsen)}));
    }
    if (a->traglastKg < 0.0) {
        a->probleme.append(neuesProblem(Schweregrad::Fehler, "WERT_NEGATIV", quelle, zeile, id,
                                        {"traglast_kg", QString::number(a->traglastKg)}));
    }
    if (!m_linienIds.contains(a->linieId)) {
        a->probleme.append(neuesProblem(Schweregrad::Fehler, "FK_LINIE", quelle, zeile, id, {a->linieId}));
    }
    if (a->hersteller.isEmpty()) {
        a->probleme.append(neuesProblem(Schweregrad::Warnung, "WERT_LEER", quelle, zeile, id, {"hersteller"}));
    }
    if (a->modell.isEmpty()) {
        a->probleme.append(neuesProblem(Schweregrad::Warnung, "WERT_LEER", quelle, zeile, id, {"modell"}));
    }

    // Die Bezeichnung beginnt mit der Linie, z.B. "L4-01 CNC-Beladung" gehoert zu L4
    QRegularExpression muster("^(L\\d+)-\\d+");
    QRegularExpressionMatch treffer = muster.match(a->bezeichnung);
    if (treffer.hasMatch() && treffer.captured(1) != a->linieId) {
        a->probleme.append(neuesProblem(Schweregrad::Warnung, "BEZEICHNUNG_LINIE", quelle, zeile, id,
                                        {a->bezeichnung, a->linieId}));
    }
}

void Datenverwaltung::anlagenVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle)
{
    const int erwartet = SPALTEN_ANLAGEN.size();

    for (const TabellenZeile& z : zeilen) {
        QStringList f = z.felder;
        const QString kandidatId = f.value(0);
        QList<Problem> leseProbleme;

        // Eine Spalte fehlt: Fehlt traglast_kg, steht an Index 8 schon das Baujahr
        if (f.size() == erwartet - 1) {
            bool istJahr = false;
            const int jahr = f.value(8).toInt(&istJahr);
            if (istJahr && jahr >= BAUJAHR_MIN && jahr < 2200) {
                leseProbleme.append(neuesProblem(Schweregrad::Warnung, "SPALTE_REPARIERT", quelle, z.nummer,
                    kandidatId, {QString::number(f.size()), QString::number(erwartet), "traglast_kg"}));
                f.insert(8, "0");
            }
        }
        if (f.size() != erwartet) {
            dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", quelle, z.nummer, kandidatId,
                         {QString::number(f.size()), QString::number(erwartet)});
            continue;
        }
        if (kandidatId.isEmpty()) {
            dateiProblem(Schweregrad::Fehler, "WERT_LEER", quelle, z.nummer, "-", {"anlage_id"});
            continue;
        }
        if (m_anlagenIds.contains(kandidatId)) {
            dateiProblem(Schweregrad::Warnung, "ID_DOPPELT", quelle, z.nummer, kandidatId, {kandidatId});
            continue;
        }

        Anlage a;
        a.anlageId = f[0];
        a.linieId = f[1];
        a.bezeichnung = f[3];
        a.typRoh = f[4];
        a.typ = anlagenTypVonText(f[4]);
        a.hersteller = f[5];
        a.modell = f[6];
        a.statusRoh = f[11];
        a.status = anlagenStatusVonText(f[11]);
        a.probleme = leseProbleme;

        double wert = 0.0;
        a.position = zahlLesen(f[2], "position", &wert, &a.probleme, quelle, z.nummer, a.anlageId) ? int(wert) : 0;
        a.achsen = zahlLesen(f[7], "achsen", &wert, &a.probleme, quelle, z.nummer, a.anlageId) ? int(wert) : 0;
        a.traglastKg = zahlLesen(f[8], "traglast_kg", &wert, &a.probleme, quelle, z.nummer, a.anlageId) ? wert : 0.0;
        a.baujahr = zahlLesen(f[9], "baujahr", &wert, &a.probleme, quelle, z.nummer, a.anlageId) ? int(wert) : 0;
        a.sollTaktzeitS = zahlLesen(f[10], "soll_taktzeit_s", &wert, &a.probleme, quelle, z.nummer, a.anlageId) ? wert : 0.0;

        anlagenWertePruefen(&a, z.nummer, quelle);

        m_anlagenIds.insert(a.anlageId);
        m_anlagen.append(a);
    }
}

void Datenverwaltung::positionenPruefen()
{
    // Vorhandene Positions-Flags entfernen und neu bestimmen
    for (Anlage& a : m_anlagen) {
        for (int i = a.probleme.size() - 1; i >= 0; i--) {
            if (a.probleme[i].code == "POSITION_DOPPELT") a.probleme.removeAt(i);
        }
    }
    QSet<QString> vergeben;
    for (Anlage& a : m_anlagen) {
        const QString schl = a.linieId + "|" + QString::number(a.position);
        if (vergeben.contains(schl)) {
            a.probleme.append(neuesProblem(Schweregrad::Warnung, "POSITION_DOPPELT", "anlagen.csv", 0,
                                           a.anlageId, {QString::number(a.position), a.linieId}));
        }
        vergeben.insert(schl);
    }
}

void Datenverwaltung::produkteVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle)
{
    for (const TabellenZeile& z : zeilen) {
        if (z.felder.size() != SPALTEN_PRODUKTE.size()) {
            dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", quelle, z.nummer, "-",
                         {QString::number(z.felder.size()), QString::number(SPALTEN_PRODUKTE.size())});
            continue;
        }
        Produkt p;
        p.produktId = z.felder[0];
        p.linieId = z.felder[1];
        p.bezeichnung = z.felder[2];

        if (p.produktId.isEmpty()) {
            dateiProblem(Schweregrad::Fehler, "WERT_LEER", quelle, z.nummer, "-", {"produkt_id"});
            continue;
        }
        if (m_produktIds.contains(p.produktId)) {
            dateiProblem(Schweregrad::Warnung, "ID_DOPPELT", quelle, z.nummer, p.produktId, {p.produktId});
            continue;
        }
        if (!m_linienIds.contains(p.linieId)) {
            p.probleme.append(neuesProblem(Schweregrad::Fehler, "FK_LINIE", quelle, z.nummer,
                                           p.produktId, {p.linieId}));
        }
        m_produktIds.insert(p.produktId);
        m_produktLinie.insert(p.produktId, p.linieId);
        m_produkte.append(p);
    }
}

void Datenverwaltung::stoercodesVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle)
{
    for (const TabellenZeile& z : zeilen) {
        if (z.felder.size() != SPALTEN_STOERCODES.size()) {
            dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", quelle, z.nummer, "-",
                         {QString::number(z.felder.size()), QString::number(SPALTEN_STOERCODES.size())});
            continue;
        }
        Stoercode s;
        s.code = z.felder[0];
        s.kategorie = z.felder[1];
        s.beschreibung = z.felder[2];

        if (s.code.isEmpty()) {
            dateiProblem(Schweregrad::Fehler, "WERT_LEER", quelle, z.nummer, "-", {"code"});
            continue;
        }
        if (m_stoercodeIds.contains(s.code)) {
            dateiProblem(Schweregrad::Warnung, "ID_DOPPELT", quelle, z.nummer, s.code, {s.code});
            continue;
        }
        if (!KATEGORIEN.contains(s.kategorie)) {
            dateiProblem(Schweregrad::Warnung, "KATEGORIE_UNBEKANNT", quelle, z.nummer, s.code, {s.kategorie});
        }
        m_stoercodeIds.insert(s.code);
        m_stoercodes.append(s);
    }
}

void Datenverwaltung::wartungenVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle)
{
    QSet<QString> bekannteIds;
    for (const TabellenZeile& z : zeilen) {
        if (z.felder.size() != SPALTEN_WARTUNGEN.size()) {
            dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", quelle, z.nummer, "-",
                         {QString::number(z.felder.size()), QString::number(SPALTEN_WARTUNGEN.size())});
            continue;
        }
        Wartung w;
        w.wartungId = z.felder[0];
        w.anlageId = z.felder[1];
        w.datum = QDate::fromString(z.felder[2], "yyyy-MM-dd");
        w.art = z.felder[3];
        w.bemerkung = z.felder[6];

        if (w.wartungId.isEmpty()) {
            dateiProblem(Schweregrad::Fehler, "WERT_LEER", quelle, z.nummer, "-", {"wartung_id"});
            continue;
        }
        if (bekannteIds.contains(w.wartungId)) {
            dateiProblem(Schweregrad::Warnung, "ID_DOPPELT", quelle, z.nummer, w.wartungId, {w.wartungId});
            continue;
        }
        bekannteIds.insert(w.wartungId);

        if (!m_anlagenIds.contains(w.anlageId)) {
            w.probleme.append(neuesProblem(Schweregrad::Fehler, "FK_ANLAGE", quelle, z.nummer,
                                           w.wartungId, {w.anlageId}));
        }
        if (!w.datum.isValid()) {
            w.probleme.append(neuesProblem(Schweregrad::Fehler, "DATUM_UNGUELTIG", quelle, z.nummer,
                                           w.wartungId, {z.felder[2]}));
        }
        if (w.art.isEmpty()) {
            w.probleme.append(neuesProblem(Schweregrad::Warnung, "WERT_LEER", quelle, z.nummer,
                                           w.wartungId, {"art"}));
        }
        bool ok = false;
        w.dauerMin = z.felder[4].toInt(&ok);
        if (!ok || w.dauerMin <= 0) {
            w.probleme.append(neuesProblem(Schweregrad::Fehler, "DAUER_UNGUELTIG", quelle, z.nummer,
                                           w.wartungId, {z.felder[4]}));
        }
        const QString geplant = z.felder[5].toLower();
        if (geplant != "ja" && geplant != "nein") {
            w.probleme.append(neuesProblem(Schweregrad::Warnung, "GEPLANT_UNGUELTIG", quelle, z.nummer,
                                           w.wartungId, {z.felder[5]}));
        }
        w.geplant = (geplant == "ja");
        m_wartungen.append(w);
    }
}

// ---------------------------------------------------------------------------
// Protokoll-Index und automatische Pruefung aller XML-Dateien
// ---------------------------------------------------------------------------
void Datenverwaltung::indexLaden()
{
    if (QFile::exists(QDir(m_ordner).filePath("master_protokoll_index.csv"))) {
        indexVerarbeiten(csvLesen("master_protokoll_index.csv", SPALTEN_INDEX.size()),
                         "master_protokoll_index.csv", QString());
        return;
    }

    // Ohne Master-Index werden die Indexdateien der einzelnen Batch-Ordner benutzt
    dateiProblem(Schweregrad::Warnung, "MASTER_FEHLT", "master_protokoll_index.csv", 0, "-");
    const QStringList batches = QDir(m_ordner).entryList({"schichtprotokolle_batch_*"},
                                                         QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& batch : batches) {
        const QString datei = batch + "/protokoll_index.csv";
        indexVerarbeiten(csvLesen(datei, SPALTEN_INDEX.size()), datei, batch + "/");
    }
}

void Datenverwaltung::indexVerarbeiten(const QList<TabellenZeile>& zeilen, const QString& quelle,
                                       const QString& pfadPraefix)
{
    for (const TabellenZeile& z : zeilen) {
        if (z.felder.size() != SPALTEN_INDEX.size()) {
            dateiProblem(Schweregrad::Fehler, "SPALTEN_ANZAHL", quelle, z.nummer, "-",
                         {QString::number(z.felder.size()), QString::number(SPALTEN_INDEX.size())});
            continue;
        }
        ProtokollIndexEintrag e;
        e.anlageId = z.felder[0];
        e.datum = QDate::fromString(z.felder[1], "yyyy-MM-dd");
        e.pfad = pfadPraefix + z.felder[2];
        e.anzahlZustaende = z.felder[3].toInt();

        if (!e.datum.isValid()) {
            dateiProblem(Schweregrad::Fehler, "DATUM_UNGUELTIG", quelle, z.nummer, e.anlageId, {z.felder[1]});
            continue;
        }
        const QString schl = schluessel(e.anlageId, e.datum);
        if (m_index.contains(schl)) {
            dateiProblem(Schweregrad::Warnung, "INDEX_DOPPELT", quelle, z.nummer, e.anlageId,
                         {e.anlageId, z.felder[1]});
            continue;
        }
        if (!m_anlagenIds.contains(e.anlageId)) {
            e.probleme.append(neuesProblem(Schweregrad::Fehler, "FK_ANLAGE", quelle, z.nummer,
                                           e.anlageId + " " + z.felder[1], {e.anlageId}));
        }
        m_index.insert(schl, e);
        m_tageJeAnlage[e.anlageId].append(e.datum);
    }
    for (auto it = m_tageJeAnlage.begin(); it != m_tageJeAnlage.end(); ++it) {
        std::sort(it.value().begin(), it.value().end());
    }
}

void Datenverwaltung::indexEintragPruefen(const QString& schl, ProtokollIndexEintrag* e)
{
    if (!e) return;
    const Anlage* anlage = findeAnlage(e->anlageId);
    const QString objektId = e->anlageId + " " + e->datum.toString("yyyy-MM-dd");

    // Nur bekannte Linien im Kontext pruefen, sonst entstehen bei fehlerhafter Linie (z.B. L9) Folgefehler
    PruefKontext kontext;
    kontext.objektId = objektId;
    kontext.anlageLinie = (anlage && m_linienIds.contains(anlage->linieId)) ? anlage->linieId : QString();
    kontext.produkte = m_produktIds;
    kontext.stoercodes = m_stoercodeIds;
    kontext.produktLinie = m_produktLinie;

    // Alte XML-Flags dieses Index-Eintrags entfernen (FK_ANLAGE aus Index bleibt erhalten)
    for (int i = e->probleme.size() - 1; i >= 0; i--) {
        if (e->probleme[i].code != "FK_ANLAGE") {
            e->probleme.removeAt(i);
        }
    }

    auto p = Protokollleser::lesen(QDir(m_ordner).filePath(e->pfad), kontext);
    e->probleme.append(p->probleme);

    if (p->geladen) {
        if (p->anlageId != e->anlageId || p->datum != e->datum) {
            e->probleme.append(neuesProblem(Schweregrad::Fehler, "XML_KOPF", e->pfad, 0, objektId,
                {p->anlageId, p->datum.toString("yyyy-MM-dd"), e->anlageId, e->datum.toString("yyyy-MM-dd")}));
        }
        if (p->anzahlZustaende() != e->anzahlZustaende) {
            e->probleme.append(neuesProblem(Schweregrad::Warnung, "XML_ANZAHL", e->pfad, 0, objektId,
                {QString::number(e->anzahlZustaende), QString::number(p->anzahlZustaende())}));
        }
        OeeKennzahlen k = OeeBerechnung::tag(*p, anlage ? anlage->sollTaktzeitS : 0.0);
        k.anlageId = e->anlageId;
        k.datum = e->datum;
        k.verlauf.clear();   // spart Speicher, wird bei Bedarf neu berechnet
        m_kennzahlen.insert(schl, k);
    }
    e->geprueft = true;
}

void Datenverwaltung::protokolleAutomatischPruefen()
{
    // Jede XML-Datei wird einmal gelesen und geprueft. Gespeichert werden nur die
    // Flags und die Tageskennzahlen; die vollstaendigen Protokolle werden spaeter
    // erst beim Oeffnen eines Tages nachgeladen (Lazy Loading).
    const int gesamt = m_index.size();
    int nummer = 0;

    for (auto it = m_index.begin(); it != m_index.end(); ++it) {
        nummer++;
        if (nummer % 10 == 0 || nummer == gesamt) emit pruefFortschritt(nummer, gesamt);

        indexEintragPruefen(it.key(), &it.value());
        m_geprueft++;
    }
}

void Datenverwaltung::problemeSammeln()
{
    m_probleme = m_dateiProbleme;
    for (const Anlage& a : m_anlagen) m_probleme.append(a.probleme);
    for (const Produkt& p : m_produkte) m_probleme.append(p.probleme);
    for (const Wartung& w : m_wartungen) m_probleme.append(w.probleme);
    for (auto it = m_index.begin(); it != m_index.end(); ++it) m_probleme.append(it.value().probleme);
}

int Datenverwaltung::anzahlProbleme(Schweregrad grad) const
{
    int anzahl = 0;
    for (const Problem& p : m_probleme) {
        if (p.grad == grad) anzahl++;
    }
    return anzahl;
}

int Datenverwaltung::anlagenProblemAnzahl(const QString& anlageId, Schweregrad grad) const
{
    int anzahl = 0;
    const Anlage* a = findeAnlage(anlageId);
    if (a) anzahl += a->anzahlProbleme(grad);
    const QList<QDate> tage = m_tageJeAnlage.value(anlageId);
    for (const QDate& tag : tage) {
        const QList<Problem> liste = m_index.value(schluessel(anlageId, tag)).probleme;
        for (const Problem& p : liste) {
            if (p.grad == grad) anzahl++;
        }
    }
    return anzahl;
}

QList<Problem> Datenverwaltung::anlagenProbleme(const QString& anlageId) const
{
    QList<Problem> liste;
    const Anlage* a = findeAnlage(anlageId);
    if (a) liste.append(a->probleme);
    const QList<QDate> tage = m_tageJeAnlage.value(anlageId);
    for (const QDate& tag : tage) {
        liste.append(m_index.value(schluessel(anlageId, tag)).probleme);
    }
    return liste;
}

QList<Problem> Datenverwaltung::tagesProbleme(const QString& anlageId, const QDate& datum) const
{
    return m_index.value(schluessel(anlageId, datum)).probleme;
}

// ---------------------------------------------------------------------------
// Zugriff auf Stammdaten
// ---------------------------------------------------------------------------
const Linie* Datenverwaltung::findeLinie(const QString& linieId) const
{
    for (const Linie& l : m_linien) {
        if (l.linieId == linieId) return &l;
    }
    return nullptr;
}

const Anlage* Datenverwaltung::findeAnlage(const QString& anlageId) const
{
    for (const Anlage& a : m_anlagen) {
        if (a.anlageId == anlageId) return &a;
    }
    return nullptr;
}

const Stoercode* Datenverwaltung::findeStoercode(const QString& code) const
{
    for (const Stoercode& s : m_stoercodes) {
        if (s.code == code) return &s;
    }
    return nullptr;
}

QList<Wartung> Datenverwaltung::wartungenVonAnlage(const QString& anlageId) const
{
    QList<Wartung> liste;
    for (const Wartung& w : m_wartungen) {
        if (w.anlageId == anlageId) liste.append(w);
    }
    return liste;
}

bool Datenverwaltung::anlageAendern(const Anlage& neu)
{
    for (Anlage& a : m_anlagen) {
        if (a.anlageId != neu.anlageId) continue;

        const bool linieGeaendert = (a.linieId != neu.linieId);
        a = neu;
        a.bearbeitet = true;
        // Nach einer Aenderung wird die Anlage erneut geprueft, alte Flags verschwinden
        a.probleme.clear();
        anlagenWertePruefen(&a, 0, "anlagen.csv");
        positionenPruefen();

        if (linieGeaendert) {
            m_cache.leeren();
            const QList<QDate> tage = m_tageJeAnlage.value(a.anlageId);
            for (const QDate& tag : tage) {
                const QString schl = schluessel(a.anlageId, tag);
                if (m_index.contains(schl)) {
                    indexEintragPruefen(schl, &m_index[schl]);
                }
            }
        } else {
            kennzahlenNeuBerechnen(a.anlageId);
        }

        problemeSammeln();
        m_trends.remove(a.anlageId);
        emit anlageGeaendert(a.anlageId);
        return true;
    }
    return false;
}

void Datenverwaltung::kennzahlenNeuBerechnen(const QString& anlageId)
{
    const Anlage* anlage = findeAnlage(anlageId);
    if (!anlage) return;
    const QList<QDate> tage = m_tageJeAnlage.value(anlageId);
    for (const QDate& tag : tage) {
        auto p = protokoll(anlageId, tag);
        if (!p->geladen) continue;
        OeeKennzahlen k = OeeBerechnung::tag(*p, anlage->sollTaktzeitS);
        k.anlageId = anlageId;
        k.datum = tag;
        k.verlauf.clear();
        m_kennzahlen.insert(schluessel(anlageId, tag), k);
    }
}

void Datenverwaltung::wartungHinzufuegen(const Wartung& wartung)
{
    m_wartungen.append(wartung);
    emit wartungHinzugefuegt(wartung.anlageId);
}

QString Datenverwaltung::naechsteWartungsId() const
{
    int hoechste = 0;
    for (const Wartung& w : m_wartungen) {
        hoechste = std::max(hoechste, w.wartungId.mid(1).toInt());
    }
    return QString("W%1").arg(hoechste + 1, 4, 10, QChar('0'));
}

// ---------------------------------------------------------------------------
// Protokolle und Auswertungen
// ---------------------------------------------------------------------------
QList<QDate> Datenverwaltung::verfuegbareTage(const QString& anlageId) const
{
    return m_tageJeAnlage.value(anlageId);
}

QList<QDate> Datenverwaltung::alleTage() const
{
    QSet<QDate> menge;
    for (auto it = m_tageJeAnlage.begin(); it != m_tageJeAnlage.end(); ++it) {
        for (const QDate& d : it.value()) menge.insert(d);
    }
    QList<QDate> liste = menge.values();
    std::sort(liste.begin(), liste.end());
    return liste;
}

QString Datenverwaltung::protokollPfad(const QString& anlageId, const QDate& datum) const
{
    return m_index.value(schluessel(anlageId, datum)).pfad;
}

std::shared_ptr<Schichtprotokoll> Datenverwaltung::protokoll(const QString& anlageId, const QDate& datum)
{
    const QString schl = schluessel(anlageId, datum);
    std::shared_ptr<Schichtprotokoll> gespeichert = m_cache.holen(schl);
    if (gespeichert) return gespeichert;

    if (!m_index.contains(schl)) {
        auto leer = std::make_shared<Schichtprotokoll>();
        leer->anlageId = anlageId;
        leer->datum = datum;
        leer->probleme.append(neuesProblem(Schweregrad::Fehler, "INDEX_FEHLT", "master_protokoll_index.csv",
                                           0, anlageId, {anlageId, datum.toString("yyyy-MM-dd")}));
        return leer;
    }

    const Anlage* anlage = findeAnlage(anlageId);
    PruefKontext kontext;
    kontext.objektId = anlageId + " " + datum.toString("yyyy-MM-dd");
    kontext.anlageLinie = (anlage && m_linienIds.contains(anlage->linieId)) ? anlage->linieId : QString();
    kontext.produkte = m_produktIds;
    kontext.stoercodes = m_stoercodeIds;
    kontext.produktLinie = m_produktLinie;

    auto geladen = Protokollleser::lesen(QDir(m_ordner).filePath(m_index.value(schl).pfad), kontext);
    if (geladen->anlageId.isEmpty()) geladen->anlageId = anlageId;
    if (!geladen->datum.isValid()) geladen->datum = datum;
    m_cache.ablegen(schl, geladen);
    return geladen;
}

OeeKennzahlen Datenverwaltung::tageskennzahlen(const QString& anlageId, const QDate& datum) const
{
    OeeKennzahlen leer;
    leer.anlageId = anlageId;
    leer.datum = datum;
    leer.bezeichnung = datum.toString("yyyy-MM-dd");
    return m_kennzahlen.value(schluessel(anlageId, datum), leer);
}

QList<OeeKennzahlen> Datenverwaltung::alleTageskennzahlen(const QString& anlageId) const
{
    QList<OeeKennzahlen> liste;
    for (const QDate& tag : m_tageJeAnlage.value(anlageId)) {
        liste.append(tageskennzahlen(anlageId, tag));
    }
    return liste;
}

Trendbericht Datenverwaltung::trendbericht(const QString& anlageId)
{
    if (m_trends.contains(anlageId)) return m_trends.value(anlageId);
    const Anlage* anlage = findeAnlage(anlageId);
    Trendbericht bericht;
    if (anlage) bericht = OeeBerechnung::trend(*anlage, alleTageskennzahlen(anlageId));
    m_trends.insert(anlageId, bericht);
    return bericht;
}

QList<EngpassEintrag> Datenverwaltung::engpass(const QString& linieId, const QDate& datum) const
{
    QList<Anlage> stationen;
    for (const Anlage& a : m_anlagen) {
        if (a.linieId == linieId) stationen.append(a);
    }
    std::sort(stationen.begin(), stationen.end(), [](const Anlage& x, const Anlage& y) {
        return x.position < y.position;
    });

    QList<OeeKennzahlen> kennzahlen;
    for (const Anlage& a : stationen) {
        kennzahlen.append(tageskennzahlen(a.anlageId, datum));
    }
    return OeeBerechnung::engpass(stationen, kennzahlen);
}

QList<ParetoEintrag> Datenverwaltung::pareto(const QString& linieId) const
{
    QList<OeeKennzahlen> alle;
    for (const Anlage& a : m_anlagen) {
        if (!linieId.isEmpty() && a.linieId != linieId) continue;
        alle.append(alleTageskennzahlen(a.anlageId));
    }
    return OeeBerechnung::pareto(alle);
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------
static QString csvFeld(const QString& text)
{
    if (text.contains(',') || text.contains('"')) {
        QString kopie = text;
        kopie.replace("\"", "\"\"");
        return "\"" + kopie + "\"";
    }
    return text;
}

bool Datenverwaltung::csvExportieren(const QString& zielOrdner, QString* fehler) const
{
    QDir ziel(zielOrdner);
    if (!ziel.exists() && !ziel.mkpath(".")) {
        if (fehler) *fehler = zielOrdner;
        return false;
    }

    QFile anlagenDatei(ziel.filePath("anlagen_out.csv"));
    if (!anlagenDatei.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (fehler) *fehler = anlagenDatei.errorString();
        return false;
    }
    QTextStream a(&anlagenDatei);
    a.setEncoding(QStringConverter::Utf8);
    a << SPALTEN_ANLAGEN.join(",") << "\n";
    for (const Anlage& an : m_anlagen) {
        a << csvFeld(an.anlageId) << "," << csvFeld(an.linieId) << "," << an.position << ","
          << csvFeld(an.bezeichnung) << ","
          << (an.typ == AnlagenTyp::Unbekannt ? an.typRoh : anlagenTypText(an.typ)) << ","
          << csvFeld(an.hersteller) << "," << csvFeld(an.modell) << "," << an.achsen << ","
          << QString::number(an.traglastKg, 'g', 8) << "," << an.baujahr << ","
          << QString::number(an.sollTaktzeitS, 'f', 1) << ","
          << (an.status == AnlagenStatus::Unbekannt ? an.statusRoh : anlagenStatusText(an.status)) << "\n";
    }

    QFile wartungsDatei(ziel.filePath("wartungen_out.csv"));
    if (!wartungsDatei.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (fehler) *fehler = wartungsDatei.errorString();
        return false;
    }
    QTextStream w(&wartungsDatei);
    w.setEncoding(QStringConverter::Utf8);
    w << SPALTEN_WARTUNGEN.join(",") << "\n";
    for (const Wartung& wa : m_wartungen) {
        w << csvFeld(wa.wartungId) << "," << csvFeld(wa.anlageId) << "," << wa.datum.toString("yyyy-MM-dd") << ","
          << csvFeld(wa.art) << "," << wa.dauerMin << "," << (wa.geplant ? "ja" : "nein") << ","
          << csvFeld(wa.bemerkung) << "\n";
    }
    return true;
}

bool Datenverwaltung::inSqliteSpeichern(const QString& sqlitePfad, QString* fehler) const
{
    const QString verbindung = "RoboCellSchreiben";
    bool erfolg = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", verbindung);
        db.setDatabaseName(sqlitePfad);
        if (!db.open()) {
            if (fehler) *fehler = db.lastError().text();
        } else {
            db.transaction();
            QSqlQuery q(db);

            q.exec("CREATE TABLE IF NOT EXISTS linien (linie_id TEXT, bezeichnung TEXT, halle TEXT, schichtmodell TEXT)");
            q.exec("DELETE FROM linien");
            q.prepare("INSERT INTO linien VALUES (?, ?, ?, ?)");
            for (const Linie& l : m_linien) {
                q.addBindValue(l.linieId); q.addBindValue(l.bezeichnung);
                q.addBindValue(l.halle); q.addBindValue(l.schichtmodell);
                q.exec();
            }

            q.exec("CREATE TABLE IF NOT EXISTS anlagen (anlage_id TEXT, linie_id TEXT, position INTEGER, "
                   "bezeichnung TEXT, typ TEXT, hersteller TEXT, modell TEXT, achsen INTEGER, "
                   "traglast_kg INTEGER, baujahr INTEGER, soll_taktzeit_s TEXT, status TEXT)");
            q.exec("DELETE FROM anlagen");
            q.prepare("INSERT INTO anlagen VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            for (const Anlage& a : m_anlagen) {
                q.addBindValue(a.anlageId); q.addBindValue(a.linieId); q.addBindValue(a.position);
                q.addBindValue(a.bezeichnung);
                q.addBindValue(a.typ == AnlagenTyp::Unbekannt ? a.typRoh : anlagenTypText(a.typ));
                q.addBindValue(a.hersteller); q.addBindValue(a.modell); q.addBindValue(a.achsen);
                q.addBindValue(a.traglastKg); q.addBindValue(a.baujahr);
                q.addBindValue(QString::number(a.sollTaktzeitS, 'f', 1));
                q.addBindValue(a.status == AnlagenStatus::Unbekannt ? a.statusRoh : anlagenStatusText(a.status));
                q.exec();
            }

            q.exec("CREATE TABLE IF NOT EXISTS produkte (produkt_id TEXT, linie_id TEXT, bezeichnung TEXT)");
            q.exec("DELETE FROM produkte");
            q.prepare("INSERT INTO produkte VALUES (?, ?, ?)");
            for (const Produkt& p : m_produkte) {
                q.addBindValue(p.produktId); q.addBindValue(p.linieId); q.addBindValue(p.bezeichnung);
                q.exec();
            }

            q.exec("CREATE TABLE IF NOT EXISTS stoercodes (code TEXT, kategorie TEXT, beschreibung TEXT)");
            q.exec("DELETE FROM stoercodes");
            q.prepare("INSERT INTO stoercodes VALUES (?, ?, ?)");
            for (const Stoercode& s : m_stoercodes) {
                q.addBindValue(s.code); q.addBindValue(s.kategorie); q.addBindValue(s.beschreibung);
                q.exec();
            }

            q.exec("CREATE TABLE IF NOT EXISTS wartungen (wartung_id TEXT, anlage_id TEXT, datum TEXT, "
                   "art TEXT, dauer_min INTEGER, geplant TEXT, bemerkung TEXT)");
            q.exec("DELETE FROM wartungen");
            q.prepare("INSERT INTO wartungen VALUES (?, ?, ?, ?, ?, ?, ?)");
            for (const Wartung& w : m_wartungen) {
                q.addBindValue(w.wartungId); q.addBindValue(w.anlageId);
                q.addBindValue(w.datum.toString("yyyy-MM-dd")); q.addBindValue(w.art);
                q.addBindValue(w.dauerMin); q.addBindValue(w.geplant ? "ja" : "nein");
                q.addBindValue(w.bemerkung);
                q.exec();
            }

            erfolg = db.commit();
            if (!erfolg && fehler) *fehler = db.lastError().text();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(verbindung);
    return erfolg;
}
