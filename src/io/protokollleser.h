#ifndef PROTOKOLLLESER_H
#define PROTOKOLLLESER_H

#include <QSet>
#include <QMap>
#include <QString>
#include <memory>
#include "model/datentypen.h"

// Angaben, mit denen ein Protokoll beim Lesen geprueft wird
struct PruefKontext {
    QString objektId;                    // z.B. "A001 2026-09-14" fuer die Problemliste
    QString anlageLinie;                 // Linie der Anlage (leer = nicht pruefen)
    QSet<QString> produkte;              // bekannte Produkt-IDs (leer = nicht pruefen)
    QSet<QString> stoercodes;            // bekannte Stoercodes (leer = nicht pruefen)
    QMap<QString, QString> produktLinie; // Produkt-ID -> Linie
};

// Liest ein XML-Schichtprotokoll mit QXmlStreamReader und prueft es dabei.
class Protokollleser {
public:
    static std::shared_ptr<Schichtprotokoll> lesen(const QString& dateiPfad,
                                                   const PruefKontext& kontext);

private:
    static void pruefeSchicht(const Schicht& schicht, const PruefKontext& kontext,
                              Schichtprotokoll* protokoll);
};

#endif // PROTOKOLLLESER_H
