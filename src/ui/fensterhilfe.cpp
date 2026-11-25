#include "fensterhilfe.h"
#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWidget>
#include <algorithm>

namespace Fensterhilfe {

static QRect verfuegbarerBereich(const QWidget* fenster)
{
    QScreen* bildschirm = fenster ? fenster->screen() : nullptr;
    if (!bildschirm) bildschirm = QGuiApplication::primaryScreen();
    if (!bildschirm) return QRect(0, 0, 1280, 720);
    return bildschirm->availableGeometry();
}

void fensterAnBildschirmAnpassen(QWidget* fenster, int wunschBreite, int wunschHoehe, double faktor)
{
    if (!fenster) return;
    const QRect bereich = verfuegbarerBereich(fenster);

    const int maxBreite = std::max(320, int(bereich.width() * faktor));
    const int maxHoehe = std::max(240, int(bereich.height() * faktor));

    // Mindestgroesse darf den Bildschirm nie ueberschreiten
    if (fenster->minimumWidth() > maxBreite || fenster->minimumHeight() > maxHoehe) {
        fenster->setMinimumSize(std::min(fenster->minimumWidth(), maxBreite),
                                std::min(fenster->minimumHeight(), maxHoehe));
    }

    const int zielBreite = std::min(wunschBreite, maxBreite);
    const int zielHoehe = std::min(wunschHoehe, maxHoehe);
    fenster->resize(zielBreite, zielHoehe);

    const int x = bereich.x() + (bereich.width() - zielBreite) / 2;
    const int y = bereich.y() + (bereich.height() - zielHoehe) / 2;
    fenster->move(x, y);
}

void geometrieSicherWiederherstellen(QWidget* fenster, const QByteArray& daten,
                                      int wunschBreite, int wunschHoehe)
{
    if (!fenster) return;

    if (!daten.isEmpty() && fenster->restoreGeometry(daten)) {
        const QRect bereich = verfuegbarerBereich(fenster);
        QRect geometrie = fenster->frameGeometry();

        // Falls gespeichertes Fenster groesser als aktueller Bildschirm oder ausserhalb liegt: anpassen
        if (!bereich.intersects(geometrie)
            || geometrie.width() > bereich.width()
            || geometrie.height() > bereich.height()) {
            fensterAnBildschirmAnpassen(fenster, wunschBreite, wunschHoehe, 0.90);
        }
        return;
    }

    fensterAnBildschirmAnpassen(fenster, wunschBreite, wunschHoehe, 0.90);
}

} // namespace Fensterhilfe
