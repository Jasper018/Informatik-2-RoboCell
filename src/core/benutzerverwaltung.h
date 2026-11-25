#ifndef BENUTZERVERWALTUNG_H
#define BENUTZERVERWALTUNG_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

// Benutzerrollen im Betrieb
enum class BenutzerRolle {
    Mitarbeiter,
    Instandhaltung,
    Admin
};

// Gespeichertes Benutzerprofil (nur Salt und Passwort-Hash, kein Klartext)
struct BenutzerProfil {
    QString name;
    BenutzerRolle rolle = BenutzerRolle::Mitarbeiter;
    QString saltHex;
    QString hashHex;
};

// Verwaltet die Benutzerprofile im Hintergrund (QSettings) und prueft die
// Anmeldung anhand von Benutzername und 16-stelligem Passwort.
class Benutzerverwaltung : public QObject {
    Q_OBJECT

public:
    explicit Benutzerverwaltung(QObject* parent = nullptr);

    // Anmeldung: erkennt Benutzername und Rolle automatisch
    bool anmelden(const QString& benutzername, const QString& passwort);
    void abmelden();

    bool istAngemeldet() const { return m_angemeldet; }
    QString aktuellerName() const { return m_aktuell.name; }
    BenutzerRolle aktuelleRolle() const { return m_aktuell.rolle; }
    QString rollenName() const;
    QString rechteBeschreibung() const;

    // Berechtigungen je Rolle
    bool darfStammdatenBearbeiten() const;
    bool darfStatusAendern() const;
    bool darfWartungErfassen() const;
    bool darfCsvExportieren() const;
    bool darfSqliteSpeichern() const;
    bool darfBerichtExportieren() const;

    // Berechnet den gesalzenen SHA-256-Hash (20.000 Runden)
    static QString passwortHash(const QString& passwort, const QByteArray& salt);

signals:
    void benutzerGeaendert();

private:
    void standardProfileSicherstellen();
    void profileAusEinstellungenLaden();

    static QString rolleZuText(BenutzerRolle rolle);
    static BenutzerRolle rolleVonText(const QString& text);
    static bool konstanterVergleich(const QString& a, const QString& b);

    QList<BenutzerProfil> m_profile;
    BenutzerProfil m_aktuell;
    bool m_angemeldet = false;
};

#endif // BENUTZERVERWALTUNG_H
