#include "io/DataRepository.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("RoboCell");
    app.setOrganizationName("DHBW_TMT25");
    app.setStyle(QStyleFactory::create("Fusion"));

    // 1. Login & Rollenauswahl (Erweiterung 3 & 9: Deutsch/Englisch)
    RoboCell::LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) {
        return 0;
    }

    // 2. Globale CSV-Daten und Master-Index beim Programmstart laden (Mindestanforderung 1)
    RoboCell::DataRepository repo;
    const QString dataDir = RoboCell::DataRepository::findDefaultDataDirectory();
    repo.loadAllFromDirectory(dataDir);

    // 3. Hauptfenster starten
    RoboCell::MainWindow mainWin(&repo);
    mainWin.show();

    return app.exec();
}
