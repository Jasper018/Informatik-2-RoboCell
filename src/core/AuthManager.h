#pragma once

#include <QObject>
#include <QString>
#include <QList>

namespace RoboCell {

/**
 * @brief Benutzerrollen gemäß Erweiterung 3 (Rollen- und Rechtekonzept)
 */
enum class UserRole {
    Mitarbeiter,     // Schichtführung / Mitarbeiter: Lesen, Filtern, OEE-Auswertung
    Instandhaltung,  // Darf zusätzlich Wartungen erfassen und Anlagenstatus ändern
    Admin            // Vollzugriff: Alle Stammdaten bearbeiten, CSV/SQLite Export, Datenreparatur
};

struct UserAccount {
    QString username;
    QString password;
    QString displayNameDe;
    QString displayNameEn;
    UserRole role;
};

/**
 * @brief Verwaltet den angemeldeten Benutzer und die Berechtigungsprüfungen.
 */
class AuthManager : public QObject {
    Q_OBJECT
public:
    static AuthManager& instance();

    bool login(const QString& username, const QString& password);
    void loginAsRole(UserRole role);

    UserRole currentRole() const { return m_currentUser.role; }
    QString currentUsername() const { return m_currentUser.username; }
    QString currentRoleDisplayName() const;
    QString permissionsSummary() const;

    // Berechtigungsprüfungen
    bool canEditMasterData() const { return m_currentUser.role == UserRole::Admin; }
    bool canChangeStatus() const {
        return m_currentUser.role == UserRole::Admin || m_currentUser.role == UserRole::Instandhaltung;
    }
    bool canRecordMaintenance() const {
        return m_currentUser.role == UserRole::Admin || m_currentUser.role == UserRole::Instandhaltung;
    }
    bool canExportData() const {
        return m_currentUser.role == UserRole::Admin || m_currentUser.role == UserRole::Instandhaltung;
    }

    const QList<UserAccount>& availableAccounts() const { return m_accounts; }

signals:
    void userChanged(UserRole newRole);

private:
    explicit AuthManager(QObject* parent = nullptr);
    QList<UserAccount> m_accounts;
    UserAccount m_currentUser;
};

} // namespace RoboCell
