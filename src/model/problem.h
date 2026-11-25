#ifndef PROBLEM_H
#define PROBLEM_H

#include <QCoreApplication>
#include <QString>
#include <QStringList>

// Schweregrad eines erkannten Datenproblems
enum class Schweregrad {
    Info,
    Warnung,
    Fehler
};

// Ein Problem ist ein "Flag", das beim Laden eines Datensatzes automatisch
// gesetzt wird. Gespeichert werden nur der Code und die Werte. Der lesbare Text
// wird erst bei der Anzeige erzeugt, damit er zur gewaehlten Sprache passt.
struct Problem {
    Q_DECLARE_TR_FUNCTIONS(Problem)

public:
    Schweregrad grad = Schweregrad::Warnung;
    QString code;       // z.B. "FK_LINIE"
    QString datei;      // z.B. "anlagen.csv"
    int zeile = 0;      // 0 = keine Zeile bekannt
    QString objektId;   // z.B. "A040"
    QStringList werte;  // Werte fuer die Platzhalter %1, %2, ...

    QString text() const;       // Beschreibung in der aktuellen Sprache
    QString gradText() const;   // "Fehler", "Warnung" oder "Info"
};

// Hilfsfunktion zum Erzeugen eines Problems
Problem neuesProblem(Schweregrad grad, const QString& code, const QString& datei,
                     int zeile, const QString& objektId, const QStringList& werte = QStringList());

#endif // PROBLEM_H
