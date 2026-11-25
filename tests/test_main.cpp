#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <cmath>
#include <cstdlib>
#include "core/benutzerverwaltung.h"
#include "core/farben.h"
#include "core/sprache.h"
#include "io/datenverwaltung.h"
#include "logic/oeeberechnung.h"
#include "model/datentypen.h"
#include "model/problem.h"

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            QTextStream err(stderr); \
            err << "TEST FEHLGESCHLAGEN (" << __FILE__ << ":" << __LINE__ << "): " #cond "\n" << Qt::flush; \
            std::exit(1); \
        } \
    } while (0)

static bool hatProblemCode(const QList<Problem>& liste, const QString& code)
{
    for (const Problem& p : liste) {
        if (p.code == code) return true;
    }
    return false;
}

static bool hatProblemFuerObjekt(const QList<Problem>& liste, const QString& objektId, const QString& code)
{
    for (const Problem& p : liste) {
        if (p.objektId == objektId && p.code == code) return true;
    }
    return false;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    out << "=== RoboCell Automatisierte Tests ===\n" << Qt::flush;

    // 1. Kontrastpruefung aller Farbpaare >= 8.0 : 1
    const QList<Farben::Farbpaar> paare = Farben::alleFarbpaare();
    CHECK(!paare.isEmpty());
    for (const Farben::Farbpaar& p : paare) {
        const double k = Farben::kontrast(p.vordergrund, p.hintergrund);
        if (k < 8.0) {
            out << "Kontrast zu niedrig bei: " << p.name << " (" << k << ")\n" << Qt::flush;
        }
        CHECK(k >= 8.0);
    }
    out << "Kontrastpruefung: alle " << paare.size() << " Farbpaare >= 8:1.\n" << Qt::flush;

    // 2. Passwort-Hash Testvektor und Benutzerrollen (keine Klartext-Passwoerter im Repo)
    const QString testVektor = Benutzerverwaltung::passwortHash(
        "Test", QByteArray::fromHex("00112233445566778899aabbccddeeff"));
    CHECK(testVektor == "3756307e3b05370d6b11a49c721420920c697fefb913731bcb7aee5cb534ec19");

    Benutzerverwaltung benutzer;
    CHECK(!benutzer.anmelden("Marlon", "Falsch1234567890"));
    CHECK(!benutzer.anmelden("Unbekannt", "Falsch1234567890"));

    // Wenn die 3 Passwoerter ueber CLI uebergeben werden, pruefen wir alle 3 Profile live
    if (argc >= 4) {
        const QString pwMarlon = QString::fromUtf8(argv[1]);
        const QString pwBastian = QString::fromUtf8(argv[2]);
        const QString pwSebastian = QString::fromUtf8(argv[3]);

        CHECK(pwMarlon.size() == 16 && pwBastian.size() == 16 && pwSebastian.size() == 16);

        // Gross-/Kleinschreibung beim Benutzernamen egal
        CHECK(benutzer.anmelden("marlon", pwMarlon));
        CHECK(benutzer.aktuelleRolle() == BenutzerRolle::Admin);
        CHECK(benutzer.darfStammdatenBearbeiten());
        CHECK(benutzer.darfStatusAendern());
        CHECK(benutzer.darfWartungErfassen());
        CHECK(benutzer.darfCsvExportieren());
        CHECK(benutzer.darfSqliteSpeichern());

        CHECK(benutzer.anmelden("Bastian", pwBastian));
        CHECK(benutzer.aktuelleRolle() == BenutzerRolle::Instandhaltung);
        CHECK(!benutzer.darfStammdatenBearbeiten());
        CHECK(benutzer.darfStatusAendern());
        CHECK(benutzer.darfWartungErfassen());
        CHECK(benutzer.darfCsvExportieren());
        CHECK(!benutzer.darfSqliteSpeichern());

        CHECK(benutzer.anmelden("Sebastian", pwSebastian));
        CHECK(benutzer.aktuelleRolle() == BenutzerRolle::Mitarbeiter);
        CHECK(!benutzer.darfStammdatenBearbeiten());
        CHECK(!benutzer.darfStatusAendern());
        CHECK(!benutzer.darfWartungErfassen());
        CHECK(!benutzer.darfCsvExportieren());
        CHECK(!benutzer.darfSqliteSpeichern());
        CHECK(benutzer.darfBerichtExportieren());
        out << "Benutzerprofile Marlon (Admin), Bastian (Instandhaltung), Sebastian (Mitarbeiter) verifiziert.\n" << Qt::flush;
    }

    // 3. Datensatz laden und automatische Flags pruefen
    Datenverwaltung daten;
    const QString ordner = Datenverwaltung::standardOrdner();
    CHECK(daten.datensatzLaden(ordner));

    CHECK(daten.linien().size() == 6);
    CHECK(daten.anlagen().size() == 82);
    CHECK(daten.produkte().size() == 21);
    CHECK(daten.stoercodes().size() == 24);
    CHECK(daten.wartungen().size() == 155);

    const QList<Problem>& flags = daten.alleProbleme();
    out << "Gepruefte Protokolltage: " << daten.anzahlProtokollTagePruefung()
        << ", erkannte Flags gesamt: " << flags.size()
        << " (Fehler: " << daten.anzahlProbleme(Schweregrad::Fehler)
        << ", Warnungen: " << daten.anzahlProbleme(Schweregrad::Warnung) << ")\n" << Qt::flush;

    // Bekannte CSV-Fehler als Flags
    CHECK(hatProblemFuerObjekt(flags, "A011", "TAKT_UNGUELTIG"));
    CHECK(hatProblemFuerObjekt(flags, "A032", "DEZIMALKOMMA"));
    CHECK(hatProblemFuerObjekt(flags, "A040", "FK_LINIE"));
    CHECK(hatProblemFuerObjekt(flags, "A060", "SPALTE_REPARIERT"));
    CHECK(hatProblemFuerObjekt(flags, "A061", "ID_DOPPELT"));
    CHECK(hatProblemFuerObjekt(flags, "A063", "TYP_UNBEKANNT"));
    CHECK(hatProblemFuerObjekt(flags, "A064", "BAUJAHR_UNPLAUSIBEL"));
    CHECK(hatProblemFuerObjekt(flags, "P-7111", "FK_LINIE"));
    CHECK(hatProblemFuerObjekt(flags, "M-102", "ID_DOPPELT"));
    CHECK(hatProblemFuerObjekt(flags, "W0019", "FK_ANLAGE"));

    // Bekannte XML-Fehler als Flags
    CHECK(hatProblemFuerObjekt(flags, "A062 2026-09-22", "DATEI_FEHLT"));
    CHECK(hatProblemFuerObjekt(flags, "A067 2026-09-18", "DATEI_FEHLT"));
    CHECK(hatProblemFuerObjekt(flags, "A045 2026-09-25", "XML_SYNTAX"));
    CHECK(hatProblemFuerObjekt(flags, "A007 2026-09-21", "ZEIT_RUECKWAERTS"));
    CHECK(hatProblemFuerObjekt(flags, "A009 2026-09-17", "STUECK_NEGATIV"));
    CHECK(hatProblemFuerObjekt(flags, "A012 2026-09-22", "CODE_UNBEKANNT"));
    CHECK(hatProblemFuerObjekt(flags, "A051 2026-09-18", "ZUSTAND_TYP"));

    // Sicherstellen, dass A040 keine Folgefehler PRODUKT_LINIE ausloest
    CHECK(!hatProblemFuerObjekt(flags, "A040 2026-09-14", "PRODUKT_LINIE"));

    // 4. OEE-, Schichtvergleich-, Engpass- und Pareto-Berechnung
    const Anlage* a001 = daten.findeAnlage("A001");
    CHECK(a001 != nullptr);
    const OeeKennzahlen k001 = daten.tageskennzahlen("A001", QDate(2026, 9, 14));
    CHECK(k001.gueltig);
    CHECK(std::abs(k001.verfuegbarkeit - 0.840) < 0.01);
    CHECK(std::abs(k001.leistungsgrad - 0.926) < 0.01);
    CHECK(std::abs(k001.qualitaet - 0.991) < 0.01);
    CHECK(std::abs(k001.oee - 0.771) < 0.01);
    CHECK(k001.gut == 1358 && k001.ausschuss == 12);

    auto p001 = daten.protokoll("A001", QDate(2026, 9, 14));
    CHECK(p001 && p001->geladen);
    CHECK(OeeBerechnung::schichten(*p001, a001->sollTaktzeitS).size() == 3);
    CHECK(daten.engpass("L1", QDate(2026, 9, 14)).size() == 14);
    CHECK(!daten.pareto("").isEmpty());

    // Korrektur an A011 muss TAKT_UNGUELTIG Flag entfernen
    const Anlage* a011 = daten.findeAnlage("A011");
    CHECK(a011 != nullptr);
    Anlage korrigiert = *a011;
    korrigiert.sollTaktzeitS = 40.0;
    CHECK(daten.anlageAendern(korrigiert));
    CHECK(!hatProblemCode(daten.findeAnlage("A011")->probleme, "TAKT_UNGUELTIG"));
    out << "OEE, Engpass, Pareto und automatische Flag-Aktualisierung verifiziert.\n" << Qt::flush;

    // 5. Export (CSV und SQLite) im temporaeren Ordner
    QTemporaryDir tmp;
    CHECK(tmp.isValid());
    QString fehler;
    CHECK(daten.csvExportieren(tmp.path(), &fehler));
    CHECK(QFile::exists(QDir(tmp.path()).filePath("anlagen_out.csv")));
    CHECK(QFile::exists(QDir(tmp.path()).filePath("wartungen_out.csv")));

    const QString sqliteDatei = QDir(tmp.path()).filePath("test.sqlite");
    CHECK(daten.inSqliteSpeichern(sqliteDatei, &fehler));
    Datenverwaltung daten2;
    CHECK(daten2.datensatzAusSqliteLaden(sqliteDatei));
    CHECK(daten2.anlagen().size() == 82);
    out << "CSV- und SQLite-Export/Import verifiziert.\n" << Qt::flush;

    // 6. Sprachumschaltung zur Laufzeit
    Sprache::setzen("en");
    CHECK(zustandName(ZustandTyp::Produktion) == "Production");
    const Problem probe = neuesProblem(Schweregrad::Fehler, "FK_LINIE", "anlagen.csv", 41, "A040", {"L9"});
    CHECK(probe.text() == "References unknown line 'L9'");
    Sprache::setzen("de");
    CHECK(zustandName(ZustandTyp::Produktion) == "Produktion");
    out << "Sprachumschaltung DE/EN verifiziert.\n" << Qt::flush;

    out << "=== ALLE TESTS ERFOLGREICH ===\n" << Qt::flush;
    return 0;
}
