#include "sprache.h"
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QTranslator>

QString Sprache::s_code = "de";
QTranslator* Sprache::s_programm = nullptr;
QTranslator* Sprache::s_qt = nullptr;

void Sprache::setzen(const QString& code)
{
    // Alte Uebersetzungen entfernen
    if (s_programm) {
        QCoreApplication::removeTranslator(s_programm);
        delete s_programm;
        s_programm = nullptr;
    }
    if (s_qt) {
        QCoreApplication::removeTranslator(s_qt);
        delete s_qt;
        s_qt = nullptr;
    }

    s_code = (code == "en") ? "en" : "de";

    if (s_code == "en") {
        // Programmtexte (Englisch)
        s_programm = new QTranslator();
        if (s_programm->load(":/i18n/RoboCell_en.qm")) {
            QCoreApplication::installTranslator(s_programm);
        }
    } else {
        // Standardtexte von Qt (Dateidialog, Buttons) auf Deutsch
        s_qt = new QTranslator();
        if (s_qt->load("qtbase_de", QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(s_qt);
        }
    }
}
