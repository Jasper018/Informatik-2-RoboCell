#include "ui/LoginDialog.h"
#include "core/LanguageManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

namespace RoboCell {

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent) {
    setMinimumWidth(460);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(24, 20, 24, 20);

    // Top bar with Language toggle
    auto* topBar = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1e293b;");
    m_langBtn = new QPushButton(this);
    m_langBtn->setCursor(Qt::PointingHandCursor);
    m_langBtn->setStyleSheet("padding: 4px 10px; font-weight: bold;");
    topBar->addWidget(m_titleLabel);
    topBar->addStretch();
    topBar->addWidget(m_langBtn);
    mainLayout->addLayout(topBar);

    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setWordWrap(true);
    m_subtitleLabel->setStyleSheet("color: #475569; margin-bottom: 6px;");
    mainLayout->addWidget(m_subtitleLabel);

    auto* formGroup = new QGroupBox(this);
    auto* formLayout = new QFormLayout(formGroup);
    formLayout->setSpacing(10);

    m_quickRoleLabel = new QLabel(this);
    m_quickRoleCombo = new QComboBox(this);
    m_quickRoleCombo->addItem("Admin (admin / admin)", static_cast<int>(UserRole::Admin));
    m_quickRoleCombo->addItem("Instandhaltung (instandhaltung / 1234)", static_cast<int>(UserRole::Instandhaltung));
    m_quickRoleCombo->addItem("Mitarbeiter / Schichtführung (mitarbeiter / 1234)", static_cast<int>(UserRole::Mitarbeiter));

    m_userLabel = new QLabel(this);
    m_userEdit = new QLineEdit("admin", this);

    m_passLabel = new QLabel(this);
    m_passEdit = new QLineEdit("admin", this);
    m_passEdit->setEchoMode(QLineEdit::Password);

    formLayout->addRow(m_quickRoleLabel, m_quickRoleCombo);
    formLayout->addRow(m_userLabel, m_userEdit);
    formLayout->addRow(m_passLabel, m_passEdit);
    mainLayout->addWidget(formGroup);

    m_permInfoLabel = new QLabel(this);
    m_permInfoLabel->setWordWrap(true);
    m_permInfoLabel->setStyleSheet("background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 8px; color: #334155;");
    mainLayout->addWidget(m_permInfoLabel);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: #dc2626; font-weight: bold;");
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    m_loginBtn = new QPushButton(this);
    m_loginBtn->setDefault(true);
    m_loginBtn->setMinimumHeight(38);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet("background-color: #2563eb; color: white; font-weight: bold; border-radius: 6px; font-size: 14px;");
    mainLayout->addWidget(m_loginBtn);

    connect(m_quickRoleCombo, &QComboBox::currentIndexChanged, this, &LoginDialog::onQuickRoleSelected);
    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_langBtn, &QPushButton::clicked, this, &LoginDialog::onLanguageToggled);
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, &LoginDialog::retranslateUi);

    retranslateUi();
    onQuickRoleSelected(0);
}

void LoginDialog::onQuickRoleSelected(int index) {
    m_errorLabel->hide();
    if (index == 0) {
        m_userEdit->setText("admin");
        m_passEdit->setText("admin");
        m_permInfoLabel->setText(trBi(
            "<b>Rolle Admin:</b> Vollzugriff auf alle Stammdaten, Bearbeitungsfunktionen, Wartungserfassung sowie CSV- und SQLite-Export.",
            "<b>Role Admin:</b> Full access to all master data, editing functions, maintenance recording, and CSV/SQLite export."
        ));
    } else if (index == 1) {
        m_userEdit->setText("instandhaltung");
        m_passEdit->setText("1234");
        m_permInfoLabel->setText(trBi(
            "<b>Rolle Instandhaltung:</b> Darf OEE & Störungen analysieren, neue Wartungen erfassen und den Anlagenstatus ändern.",
            "<b>Role Maintenance:</b> Can analyze OEE & faults, record new maintenance entries, and change machine status."
        ));
    } else {
        m_userEdit->setText("mitarbeiter");
        m_passEdit->setText("1234");
        m_permInfoLabel->setText(trBi(
            "<b>Rolle Mitarbeiter / Schichtführung:</b> Lesezugriff auf alle Linien, Anlagen, Schichtprotokolle und OEE-Auswertungen.",
            "<b>Role Employee / Shift Lead:</b> Read access to all lines, machines, shift logs, and OEE evaluations."
        ));
    }
}

void LoginDialog::onLoginClicked() {
    if (AuthManager::instance().login(m_userEdit->text(), m_passEdit->text())) {
        accept();
    } else {
        m_errorLabel->setText(trBi(
            "Ungültiger Benutzername oder Passwort! (Tipp: Profil oben auswählen)",
            "Invalid username or password! (Tip: Select profile above)"
        ));
        m_errorLabel->show();
    }
}

void LoginDialog::onLanguageToggled() {
    auto& lm = LanguageManager::instance();
    lm.setLanguage(lm.currentLanguage() == AppLanguage::German ? AppLanguage::English : AppLanguage::German);
}

void LoginDialog::retranslateUi() {
    setWindowTitle(trBi("RoboCell – Anmeldung & Rollenauswahl", "RoboCell – Login & Role Selection"));
    m_titleLabel->setText(trBi("RoboCell OEE-Management", "RoboCell OEE Management"));
    m_langBtn->setText(LanguageManager::instance().currentLanguage() == AppLanguage::German ? "🌐 EN (English)" : "🌐 DE (Deutsch)");
    m_subtitleLabel->setText(trBi(
        "Bitte melden Sie sich an oder wählen Sie ein Rollenprofil für die Sitzung aus (Benutzerwechsel ist jederzeit auch im Hauptfenster möglich):",
        "Please sign in or select a role profile for your session (you can also switch users at any time in the main window):"
    ));
    m_quickRoleLabel->setText(trBi("Schnellauswahl Rolle:", "Quick Role Select:"));
    m_userLabel->setText(trBi("Benutzername:", "Username:"));
    m_passLabel->setText(trBi("Passwort:", "Password:"));
    m_loginBtn->setText(trBi("Anmelden & Starten", "Sign In & Start"));

    m_quickRoleCombo->setItemText(0, trBi("Administrator (admin / admin)", "Administrator (admin / admin)"));
    m_quickRoleCombo->setItemText(1, trBi("Instandhaltung (instandhaltung / 1234)", "Maintenance (instandhaltung / 1234)"));
    m_quickRoleCombo->setItemText(2, trBi("Mitarbeiter / Schichtführung (mitarbeiter / 1234)", "Employee / Shift Lead (mitarbeiter / 1234)"));
    onQuickRoleSelected(m_quickRoleCombo->currentIndex());
}

} // namespace RoboCell
