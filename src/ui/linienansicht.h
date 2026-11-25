#ifndef LINIENANSICHT_H
#define LINIENANSICHT_H

#include <QWidget>

class QComboBox;
class QEvent;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QSlider;
class QTableWidget;
class Datenverwaltung;

// Grafische Linienansicht (QGraphicsView) mit Zeitschieberegler und Engpassanalyse
class LinienAnsichtWidget : public QWidget {
    Q_OBJECT

public:
    explicit LinienAnsichtWidget(Datenverwaltung* daten, QWidget* parent = nullptr);

    void aktualisieren();

signals:
    void anlageDoppelklick(const QString& anlageId);

protected:
    bool eventFilter(QObject* objekt, QEvent* ereignis) override;
    void changeEvent(QEvent* ereignis) override;

private slots:
    void slotAuswahlGeaendert();
    void slotZeitGeaendert(int minutenAbSechs);

private:
    void texteAktualisieren();
    void szeneZeichnen();

    Datenverwaltung* m_daten = nullptr;

    QLabel* m_linieLabel = nullptr;
    QComboBox* m_linieCombo = nullptr;
    QLabel* m_tagLabel = nullptr;
    QComboBox* m_tagCombo = nullptr;
    QLabel* m_modusLabel = nullptr;
    QComboBox* m_modusCombo = nullptr;
    QLabel* m_uhrzeitLabel = nullptr;
    QSlider* m_zeitSlider = nullptr;
    QLabel* m_uhrzeitWertLabel = nullptr;
    QLabel* m_engpassBanner = nullptr;

    QGraphicsScene* m_szene = nullptr;
    QGraphicsView* m_ansicht = nullptr;
    QTableWidget* m_engpassTabelle = nullptr;
};

#endif // LINIENANSICHT_H
