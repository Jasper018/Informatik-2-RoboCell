#ifndef ANLAGENMODELL_H
#define ANLAGENMODELL_H

#include <QAbstractTableModel>
#include <QList>
#include <QSortFilterProxyModel>
#include "logic/oeeberechnung.h"
#include "model/datentypen.h"

class Datenverwaltung;

// Tabellenmodell fuer die Anlagenliste (QTableView + QAbstractTableModel)
// Helt fuer fluessiges Scrollen einen vorab berechneten Zeilen-Cache vor.
class AnlagenModell : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Spalte {
        SpAnlageId = 0,
        SpLinieId,
        SpPosition,
        SpBezeichnung,
        SpTyp,
        SpHerstellerModell,
        SpBaujahr,
        SpSollTakt,
        SpStatus,
        SpMittelOee,
        SpStoerzeitMin,
        SpFlags,
        SpaltenAnzahl
    };

    enum BenutzerRollen {
        SortierWertRolle = Qt::UserRole + 1,
        AnlageIdRolle,
        LinieIdRolle,
        TypRolle,
        StatusRolle,
        BaujahrRolle,
        AuffaelligRolle,
        HatFlagRolle
    };

    explicit AnlagenModell(Datenverwaltung* daten, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void aktualisieren();
    void spracheAktualisieren();
    void farbenAktualisieren();

private:
    struct ZeilenCache {
        Trendbericht bericht;
        int fehlerAnz = 0;
        int warnAnz = 0;
        bool hatFlag = false;
        QString herstellerModell;
        QString oeeText;
        QString stoerzeitText;
    };

    void cacheNeuBauen();

    Datenverwaltung* m_daten = nullptr;
    QList<ZeilenCache> m_zeilen;
};

// Filter- und Sortiermodell fuer die Anlagenliste
class AnlagenFilterModell : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit AnlagenFilterModell(QObject* parent = nullptr);

    void setSuchtext(const QString& text);
    void setLinieFilter(const QString& linieId);
    void setTypFilter(int typOderMinus1);
    void setStatusFilter(int statusOderMinus1);
    void setMaxBaujahr(int jahrOder0);
    void setNurAuffaellige(bool nur);
    void setNurMitFlag(bool nur);
    void filterZuruecksetzen();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    QString m_suchtext;
    QString m_linie;
    int m_typ = -1;
    int m_status = -1;
    int m_maxBaujahr = 0;
    bool m_nurAuffaellig = false;
    bool m_nurMitFlag = false;
};

#endif // ANLAGENMODELL_H
