#pragma once

#include <QObject>
#include <QString>
#include <QMap>

namespace RoboCell {

enum class AppLanguage {
    German,
    English
};

/**
 * @brief Verwaltet die Mehrsprachigkeit (Deutsch / Englisch) mit Laufzeit-Umschaltung (Erweiterung 9).
 */
class LanguageManager : public QObject {
    Q_OBJECT
public:
    static LanguageManager& instance();

    AppLanguage currentLanguage() const { return m_lang; }
    void setLanguage(AppLanguage lang);

    /**
     * @briefGibt je nach gewählter Sprache den deutschen oder englischen Text zurück.
     */
    QString pick(const QString& de, const QString& en) const {
        return (m_lang == AppLanguage::German) ? de : en;
    }

signals:
    void languageChanged(AppLanguage newLang);

private:
    explicit LanguageManager(QObject* parent = nullptr);
    AppLanguage m_lang = AppLanguage::German;
};

// Komfort-Funktion für zweisprachige Strings im gesamten Projekt
inline QString trBi(const QString& de, const QString& en) {
    return LanguageManager::instance().pick(de, en);
}

} // namespace RoboCell
