#include "core/AuthManager.h"
#include "core/LanguageManager.h"

namespace RoboCell {

AuthManager::AuthManager(QObject* parent) : QObject(parent) {
    m_accounts = {
        {"mitarbeiter", "1234", "Mitarbeiter / Schichtführung", "Employee / Shift Lead", UserRole::Mitarbeiter},
        {"instandhaltung", "1234", "Instandhaltung / Techniker", "Maintenance Technician", UserRole::Instandhaltung},
        {"admin", "admin", "Administrator (Vollzugriff)", "Administrator (Full Access)", UserRole::Admin}
    };
    m_currentUser = m_accounts.last(); // Default Admin falls ohne Dialog gestartet (z.B. in Tests)
}

AuthManager& AuthManager::instance() {
    static AuthManager inst;
    return inst;
}

bool AuthManager::login(const QString& username, const QString& password) {
    const QString u = username.trimmed().toLower();
    for (const auto& acc : m_accounts) {
        if (acc.username.toLower() == u && acc.password == password) {
            m_currentUser = acc;
            emit userChanged(m_currentUser.role);
            return true;
        }
    }
    return false;
}

void AuthManager::loginAsRole(UserRole role) {
    for (const auto& acc : m_accounts) {
        if (acc.role == role) {
            m_currentUser = acc;
            emit userChanged(m_currentUser.role);
            return;
        }
    }
}

QString AuthManager::currentRoleDisplayName() const {
    return trBi(m_currentUser.displayNameDe, m_currentUser.displayNameEn);
}

QString AuthManager::permissionsSummary() const {
    switch (m_currentUser.role) {
        case UserRole::Mitarbeiter:
            return trBi("Lesen, Suchen/Filtern, Protokoll- & OEE-Auswertung (Schreibgeschützt)",
                        "Read, Search/Filter, Log & OEE Analysis (Read-Only)");
        case UserRole::Instandhaltung:
            return trBi("OEE-Auswertung + Wartungen erfassen + Anlagenstatus ändern",
                        "OEE Analysis + Record Maintenance + Change Machine Status");
        case UserRole::Admin:
            return trBi("Vollzugriff: Alle Stammdaten bearbeiten, Wartungen erfassen, CSV/SQLite-Export",
                        "Full Access: Edit All Master Data, Record Maintenance, CSV/SQLite Export");
    }
    return QString();
}

} // namespace RoboCell
