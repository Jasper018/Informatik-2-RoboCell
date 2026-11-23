#include "core/LanguageManager.h"

namespace RoboCell {

LanguageManager::LanguageManager(QObject* parent)
    : QObject(parent), m_lang(AppLanguage::German) {}

LanguageManager& LanguageManager::instance() {
    static LanguageManager inst;
    return inst;
}

void LanguageManager::setLanguage(AppLanguage lang) {
    if (m_lang != lang) {
        m_lang = lang;
        emit languageChanged(m_lang);
    }
}

} // namespace RoboCell
