#pragma once

#include "model/Entities.h"
#include "logic/OeeCalculator.h"
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QMap>

namespace RoboCell {

class DataRepository;

/**
 * @brief Tabellenmodell für die Anlagenliste (Mindestanforderung 2) inkl. berechneter OEE- und Trend-Spalten.
 */
class AnlagenTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColAnlageId = 0,
        ColLinieId,
        ColPosition,
        ColBezeichnung,
        ColTyp,
        ColHerstellerModell,
        ColBaujahr,
        ColSollTaktzeit,
        ColStatus,
        ColAvgOee,
        ColStoerzeitMin,
        ColDataQuality,
        ColumnCount
    };

    explicit AnlagenTableModel(DataRepository* repo, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refreshModel();
    void updateCalculatedMetrics(const QMap<QString, AnomalyReport>& reports);

    const Anlage* anlageAtRow(int row) const;
    const AnomalyReport* anomalyForAnlage(const QString& anlageId) const;

    enum CustomRoles {
        SortValueRole = Qt::UserRole + 1,
        AnlageIdRole = Qt::UserRole + 2,
        LinieIdRole = Qt::UserRole + 3,
        TypEnumRole = Qt::UserRole + 4,
        StatusEnumRole = Qt::UserRole + 5,
        BaujahrRole = Qt::UserRole + 6,
        SollTaktRole = Qt::UserRole + 7,
        IsConspicuousRole = Qt::UserRole + 8,
        HasDataWarningRole = Qt::UserRole + 9
    };

private:
    DataRepository* m_repo = nullptr;
    QMap<QString, AnomalyReport> m_anomalyReports;
};

/**
 * @brief Mehrstufiges Such-, Filter- und Sortier-Proxy-Modell (Mindestanforderung 3 & Erweiterung 2).
 */
class AnlagenFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit AnlagenFilterProxyModel(QObject* parent = nullptr);

    void setSearchText(const QString& text);
    void setLinieFilter(const QString& linieId);
    void setTypFilter(int typEnumOrMinus1);
    void setStatusFilter(int statusEnumOrMinus1);
    void setMaxBaujahrFilter(int maxYearOr0);
    void setMinBaujahrFilter(int minYearOr0);
    void setOnlyConspicuous(bool only);
    void setOnlyDataWarnings(bool only);
    void resetAllFilters();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const override;

private:
    QString m_searchText;
    QString m_linieFilter;
    int m_typFilter = -1;
    int m_statusFilter = -1;
    int m_minBaujahr = 0;
    int m_maxBaujahr = 0;
    bool m_onlyConspicuous = false;
    bool m_onlyDataWarnings = false;
};

} // namespace RoboCell
