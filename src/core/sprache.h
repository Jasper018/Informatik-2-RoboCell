#ifndef SPRACHE_H
#define SPRACHE_H

#include <QString>

class QTranslator;

// Umschaltung der Oberflaechensprache zur Laufzeit (Deutsch / Englisch).
// Der Quelltext ist deutsch. Fuer Englisch wird eine Uebersetzungsdatei geladen.
// Qt schickt danach an jedes Fenster ein LanguageChange-Ereignis.
class Sprache {
public:
    static void setzen(const QString& code);   // "de" oder "en"
    static QString aktuell() { return s_code; }

private:
    static QString s_code;
    static QTranslator* s_programm;
    static QTranslator* s_qt;
};

#endif // SPRACHE_H
