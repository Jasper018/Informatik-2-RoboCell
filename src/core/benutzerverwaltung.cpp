#include "benutzerverwaltung.h"
#include <QCryptographicHash>
#include <QSettings>

static const int HASH_RUNDEN = 20000;

Benutzerverwaltung::Benutzerverwaltung(QObject* parent)
    : QObject(parent)
{
    standardProfileSicherstellen();
    profileAusEinstellungenLaden();
}

void Benutzerverwaltung::standardProfileSicherstellen()
{
    QSettings s("DHBW_TMT25", "RoboCell");
    s.beginGroup("BenutzerProfile");

    struct Vorgabe {
        const char* name;
        const char* rolle;
        const char* salt;
        const char* hash;
    };

    // Gespeicherte Profile (Passwoerter liegen nur als gesalzener SHA-256-Hash vor)
    const Vorgabe vorgaben[3] = {
        {
            "Marlon", "admin",
            "5b6d75c2e301e1b3eb02102e8fd10789",
            "37d0afc60a8d8a375c4cd37e587a66c71fc11e963ed3b0e4b3c8e50c986e521f"
        },
        {
            "Bastian", "instandhaltung",
            "31c38b7d0cdaed551ed8bb4a4fa5233a",
            "e9348981a4449ee4fdf57e7b47455f57f1c68783a1c901b227baeb541bce6573"
        },
        {
            "Sebastian", "mitarbeiter",
            "a0198b39a309a4c846109e318286d937",
            "178dfd26d57bb62b2b3578be1997c6e79f1443ea6673fa3442c2dc9f905c1fbb"
        }
    };

    for (int i = 0; i < 3; i++) {
        const QString name = QString::fromLatin1(vorgaben[i].name);
        s.beginGroup(name);
        if (!s.contains("hash") || !s.contains("salt") || !s.contains("rolle")) {
            s.setValue("rolle", QString::fromLatin1(vorgaben[i].rolle));
            s.setValue("salt", QString::fromLatin1(vorgaben[i].salt));
            s.setValue("hash", QString::fromLatin1(vorgaben[i].hash));
        }
        s.endGroup();
    }

    s.endGroup();
    s.sync();
}

void Benutzerverwaltung::profileAusEinstellungenLaden()
{
    m_profile.clear();
    QSettings s("DHBW_TMT25", "RoboCell");
    s.beginGroup("BenutzerProfile");
    const QStringList namen = s.childGroups();
    for (const QString& name : namen) {
        s.beginGroup(name);
        BenutzerProfil p;
        p.name = name;
        p.rolle = rolleVonText(s.value("rolle").toString());
        p.saltHex = s.value("salt").toString();
        p.hashHex = s.value("hash").toString();
        m_profile.append(p);
        s.endGroup();
    }
    s.endGroup();
}

QString Benutzerverwaltung::passwortHash(const QString& passwort, const QByteArray& salt)
{
    const QByteArray daten = salt + passwort.toUtf8();
    QByteArray hash = QCryptographicHash::hash(daten, QCryptographicHash::Sha256);
    for (int i = 0; i < HASH_RUNDEN; i++) {
        hash = QCryptographicHash::hash(hash + daten, QCryptographicHash::Sha256);
    }
    return QString::fromLatin1(hash.toHex());
}

bool Benutzerverwaltung::konstanterVergleich(const QString& a, const QString& b)
{
    if (a.size() != b.size()) return false;
    int unterschied = 0;
    for (int i = 0; i < a.size(); i++) {
        unterschied |= (a.at(i).unicode() ^ b.at(i).unicode());
    }
    return unterschied == 0;
}

bool Benutzerverwaltung::anmelden(const QString& benutzername, const QString& passwort)
{
    const QString gesucht = benutzername.trimmed();
    if (gesucht.isEmpty() || passwort.isEmpty()) return false;

    for (const BenutzerProfil& p : m_profile) {
        if (p.name.compare(gesucht, Qt::CaseInsensitive) != 0) continue;

        const QByteArray salt = QByteArray::fromHex(p.saltHex.toLatin1());
        const QString berechnet = passwortHash(passwort, salt);
        if (konstanterVergleich(berechnet, p.hashHex)) {
            m_aktuell = p;
            m_angemeldet = true;
            emit benutzerGeaendert();
            return true;
        }
        return false;
    }
    return false;
}

void Benutzerverwaltung::abmelden()
{
    m_angemeldet = false;
    m_aktuell = BenutzerProfil();
    emit benutzerGeaendert();
}

QString Benutzerverwaltung::rollenName() const
{
    switch (m_aktuell.rolle) {
    case BenutzerRolle::Admin:
        return tr("Administrator");
    case BenutzerRolle::Instandhaltung:
        return tr("Instandhaltung");
    case BenutzerRolle::Mitarbeiter:
        return tr("Mitarbeiter");
    }
    return tr("Mitarbeiter");
}

QString Benutzerverwaltung::rechteBeschreibung() const
{
    switch (m_aktuell.rolle) {
    case BenutzerRolle::Admin:
        return tr("Vollzugriff: Alle Stammdaten bearbeiten, Status ändern, Wartungen erfassen, CSV und SQLite exportieren.");
    case BenutzerRolle::Instandhaltung:
        return tr("Instandhaltung: Auswertungen lesen, Anlagenstatus ändern, Wartungen erfassen und CSV exportieren.");
    case BenutzerRolle::Mitarbeiter:
        return tr("Mitarbeiter: Lesezugriff auf Linien, Anlagen, Protokolle, OEE-Auswertungen und Berichte (keine Stammdatenänderung).");
    }
    return QString();
}

bool Benutzerverwaltung::darfStammdatenBearbeiten() const
{
    return m_angemeldet && m_aktuell.rolle == BenutzerRolle::Admin;
}

bool Benutzerverwaltung::darfStatusAendern() const
{
    return m_angemeldet && (m_aktuell.rolle == BenutzerRolle::Admin
                            || m_aktuell.rolle == BenutzerRolle::Instandhaltung);
}

bool Benutzerverwaltung::darfWartungErfassen() const
{
    return m_angemeldet && (m_aktuell.rolle == BenutzerRolle::Admin
                            || m_aktuell.rolle == BenutzerRolle::Instandhaltung);
}

bool Benutzerverwaltung::darfCsvExportieren() const
{
    return m_angemeldet && (m_aktuell.rolle == BenutzerRolle::Admin
                            || m_aktuell.rolle == BenutzerRolle::Instandhaltung);
}

bool Benutzerverwaltung::darfSqliteSpeichern() const
{
    return m_angemeldet && m_aktuell.rolle == BenutzerRolle::Admin;
}

bool Benutzerverwaltung::darfBerichtExportieren() const
{
    return m_angemeldet;
}

QString Benutzerverwaltung::rolleZuText(BenutzerRolle rolle)
{
    switch (rolle) {
    case BenutzerRolle::Admin: return "admin";
    case BenutzerRolle::Instandhaltung: return "instandhaltung";
    case BenutzerRolle::Mitarbeiter: return "mitarbeiter";
    }
    return "mitarbeiter";
}

BenutzerRolle Benutzerverwaltung::rolleVonText(const QString& text)
{
    const QString t = text.trimmed().toLower();
    if (t == "admin") return BenutzerRolle::Admin;
    if (t == "instandhaltung") return BenutzerRolle::Instandhaltung;
    return BenutzerRolle::Mitarbeiter;
}
