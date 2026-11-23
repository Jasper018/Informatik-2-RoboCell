#pragma once

#include "core/AuthManager.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

namespace RoboCell {

/**
 * @brief Login- und Rollenauswahl-Dialog mit Sprachumschaltung (Erweiterung 3 & 9).
 */
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);

private slots:
    void onQuickRoleSelected(int index);
    void onLoginClicked();
    void onLanguageToggled();
    void retranslateUi();

private:
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_quickRoleLabel = nullptr;
    QComboBox* m_quickRoleCombo = nullptr;
    QLabel* m_userLabel = nullptr;
    QLineEdit* m_userEdit = nullptr;
    QLabel* m_passLabel = nullptr;
    QLineEdit* m_passEdit = nullptr;
    QLabel* m_permInfoLabel = nullptr;
    QLabel* m_errorLabel = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QPushButton* m_langBtn = nullptr;
};

} // namespace RoboCell
