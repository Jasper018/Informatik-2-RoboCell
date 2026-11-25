#ifndef ANMELDEDIALOG_H
#define ANMELDEDIALOG_H

#include <QDialog>

class QCheckBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class Benutzerverwaltung;

// Anmeldefenster: Benutzername und Passwort eingeben.
// Die Rolle wird automatisch ueber das im Hintergrund gespeicherte Profil ermittelt.
class AnmeldeDialog : public QDialog {
    Q_OBJECT

public:
    explicit AnmeldeDialog(Benutzerverwaltung* benutzer, QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* ereignis) override;

private slots:
    void slotAnmelden();
    void slotPasswortAnzeigen(bool sichtbar);

private:
    void texteAktualisieren();

    Benutzerverwaltung* m_benutzer = nullptr;

    QLabel* m_titelLabel = nullptr;
    QLabel* m_hinweisLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QLabel* m_passwortLabel = nullptr;
    QLineEdit* m_passwortEdit = nullptr;
    QCheckBox* m_zeigenCheck = nullptr;
    QLabel* m_fehlerLabel = nullptr;
    QPushButton* m_abbrechenBtn = nullptr;
    QPushButton* m_anmeldenBtn = nullptr;
};

#endif // ANMELDEDIALOG_H
