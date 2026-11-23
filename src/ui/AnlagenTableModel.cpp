#include "ui/AnlagenTableModel.h"
#include "io/DataRepository.h"
#include "core/LanguageManager.h"
#include <QColor>
#include <QBrush>
#include <QFont>

namespace RoboCell {

AnlagenTableModel::AnlagenTableModel(DataRepository* repo, QObject* parent)
    : QAbstractTableModel(parent), m_repo(repo)
{
    connect(&LanguageManager::instance(), &LanguageManager::languageChanged, this, [this]() {
        emit headerDataChanged(Qt::Horizontal, 0, ColumnCount - 1);
        if (rowCount() > 0) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
        }
    });
}

int AnlagenTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || !m_repo) return 0;
    return m_repo->anlagen().size();
}

int AnlagenTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColumnCount;
}

const Anlage* AnlagenTableModel::anlageAtRow(int row) const {
    if (!m_repo || row < 0 || row >= m_repo->anlagen().size()) return nullptr;
    return &m_repo->anlagen().at(row);
}

const AnomalyReport* AnlagenTableModel::anomalyForAnlage(const QString& anlageId) const {
    auto it = m_anomalyReports.find(anlageId);
    if (it != m_anomalyReports.end()) return &it.value();
    return nullptr;
}

void AnlagenTableModel::refreshModel() {
    beginResetModel();
    endResetModel();
}

void AnlagenTableModel::updateCalculatedMetrics(const QMap<QString, AnomalyReport>& reports) {
    m_anomalyReports = reports;
    if (rowCount() > 0) {
        emit dataChanged(index(0, ColAvgOee), index(rowCount() - 1, ColDataQuality));
    }
}

QVariant AnlagenTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();

    switch (section) {
        case ColAnlageId: return trBi("Anlage-ID", "Machine ID");
        case ColLinieId: return trBi("Linie", "Line");
        case ColPosition: return trBi("Pos.", "Pos.");
        case ColBezeichnung: return trBi("Bezeichnung", "Description");
        case ColTyp: return trBi("Typ", "Type");
        case ColHerstellerModell: return trBi("Hersteller / Modell", "Vendor / Model");
        case ColBaujahr: return trBi("Baujahr", "Year");
        case ColSollTaktzeit: return trBi("Soll-Takt [s]", "Cycle Time [s]");
        case ColStatus: return trBi("Status", "Status");
        case ColAvgOee: return trBi("Ø OEE (14T)", "Avg OEE (14d)");
        case ColStoerzeitMin: return trBi("Störzeit [min]", "Fault Time [min]");
        case ColDataQuality: return trBi("Diagnose / Status", "Diagnostics");
    }
    return QVariant();
}

QVariant AnlagenTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !m_repo) return QVariant();
    const int row = index.row();
    const int col = index.column();
    if (row < 0 || row >= m_repo->anlagen().size()) return QVariant();

    const Anlage& a = m_repo->anlagen().at(row);
    const AnomalyReport* rep = anomalyForAnlage(a.anlageId);

    // Custom Filter & Sort Roles
    if (role == AnlageIdRole) return a.anlageId;
    if (role == LinieIdRole) return a.linieId;
    if (role == TypEnumRole) return static_cast<int>(a.typ);
    if (role == StatusEnumRole) return static_cast<int>(a.status);
    if (role == BaujahrRole) return a.baujahr;
    if (role == SollTaktRole) return a.sollTaktzeitS;
    if (role == IsConspicuousRole) return rep ? rep->isConspicuous : false;
    if (role == HasDataWarningRole) return a.hasValidationWarning;

    if (role == SortValueRole) {
        switch (col) {
            case ColAnlageId: return a.anlageId;
            case ColLinieId: return a.linieId;
            case ColPosition: return a.position;
            case ColBezeichnung: return a.bezeichnung;
            case ColTyp: return anlagenTypToCsv(a.typ, a.rawTyp);
            case ColHerstellerModell: return a.hersteller + " " + a.modell;
            case ColBaujahr: return a.baujahr;
            case ColSollTaktzeit: return a.sollTaktzeitS;
            case ColStatus: return static_cast<int>(a.status);
            case ColAvgOee: return rep ? rep->avgOee : -1.0;
            case ColStoerzeitMin: return rep ? (static_cast<double>(rep->totalStoerSek) / 60.0) : -1.0;
            case ColDataQuality:
                return (a.hasValidationWarning ? 2 : 0) + ((rep && rep->isConspicuous) ? 1 : 0);
        }
    }

    if (role == Qt::DisplayRole) {
        switch (col) {
            case ColAnlageId: return a.anlageId;
            case ColLinieId: return a.linieId;
            case ColPosition: return a.position;
            case ColBezeichnung: return a.bezeichnung;
            case ColTyp: return anlagenTypToCsv(a.typ, a.rawTyp);
            case ColHerstellerModell: return QString("%1 %2").arg(a.hersteller, a.modell);
            case ColBaujahr: return a.baujahr;
            case ColSollTaktzeit: return QString::number(a.sollTaktzeitS, 'f', 1);
            case ColStatus: {
                switch (a.status) {
                    case AnlagenStatus::InBetrieb: return trBi("in_betrieb", "in_operation");
                    case AnlagenStatus::Stoerung: return trBi("stoerung", "fault");
                    case AnlagenStatus::Wartung: return trBi("wartung", "maintenance");
                    case AnlagenStatus::Umbau: return trBi("umbau", "conversion");
                    case AnlagenStatus::Unbekannt: return a.rawStatus;
                }
                return a.rawStatus;
            }
            case ColAvgOee:
                if (!rep || rep->avgOee <= 0.0) return trBi("Lade...", "Loading...");
                return QString("%1 %").arg(rep->avgOee * 100.0, 0, 'f', 1);
            case ColStoerzeitMin:
                if (!rep) return "-";
                return QString("%1 min").arg(static_cast<double>(rep->totalStoerSek) / 60.0, 0, 'f', 0);
            case ColDataQuality: {
                QStringList flags;
                if (a.hasValidationWarning) flags << trBi("⚠ Datenfehler", "⚠ Data Issue");
                if (rep && rep->isConspicuous) flags << trBi("🔍 Auffällig", "🔍 Conspicuous");
                if (a.isModified) flags << trBi("✎ Bearbeitet", "✎ Edited");
                return flags.isEmpty() ? trBi("✓ OK", "✓ OK") : flags.join(" | ");
            }
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (col == ColPosition || col == ColBaujahr || col == ColSollTaktzeit ||
            col == ColAvgOee || col == ColStoerzeitMin) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        }
        if (col == ColAnlageId || col == ColLinieId || col == ColStatus) {
            return int(Qt::AlignCenter);
        }
    }

    if (role == Qt::BackgroundRole) {
        if (col == ColStatus) {
            switch (a.status) {
                case AnlagenStatus::InBetrieb: return QBrush(QColor("#dcfce7"));
                case AnlagenStatus::Stoerung: return QBrush(QColor("#fee2e2"));
                case AnlagenStatus::Wartung: return QBrush(QColor("#fef9c3"));
                case AnlagenStatus::Umbau: return QBrush(QColor("#e0e7ff"));
                default: break;
            }
        }
        if (col == ColAvgOee && rep && rep->avgOee > 0.0) {
            if (rep->avgOee < 0.70) return QBrush(QColor("#fee2e2"));
            if (rep->avgOee >= 0.80) return QBrush(QColor("#dcfce7"));
            return QBrush(QColor("#fef9c3"));
        }
        if (a.hasValidationWarning && col == ColDataQuality) {
            return QBrush(QColor("#ffedd5"));
        }
    }

    if (role == Qt::ForegroundRole) {
        if (col == ColStatus) {
            switch (a.status) {
                case AnlagenStatus::InBetrieb: return QBrush(QColor("#166534"));
                case AnlagenStatus::Stoerung: return QBrush(QColor("#991b1b"));
                case AnlagenStatus::Wartung: return QBrush(QColor("#854d0e"));
                case AnlagenStatus::Umbau: return QBrush(QColor("#1e40af"));
                default: break;
            }
        }
        if (col == ColDataQuality && a.hasValidationWarning) {
            return QBrush(QColor("#c2410c"));
        }
    }

    if (role == Qt::FontRole) {
        if (col == ColAnlageId || col == ColStatus || col == ColAvgOee) {
            QFont f;
            f.setBold(true);
            return f;
        }
    }

    if (role == Qt::ToolTipRole) {
        QStringList tip;
        tip << QString("<b>%1 – %2</b>").arg(a.anlageId, a.bezeichnung);
        tip << trBi("Linie: %1 (Position %2)", "Line: %1 (Position %2)").arg(a.linieId).arg(a.position);
        tip << trBi("Hersteller/Modell: %1 %2 (%3 Achsen, %4 kg)",
                    "Vendor/Model: %1 %2 (%3 Axes, %4 kg)")
                   .arg(a.hersteller, a.modell).arg(a.achsen).arg(a.traglastKg);
        if (a.hasValidationWarning) {
            tip << "<hr><b>" + trBi("Stammdaten-Hinweise:", "Master Data Warnings:") + "</b>";
            for (const QString& m : a.validationMessages) {
                tip << "• " + m;
            }
        }
        if (rep && (!rep->summaryNotesDe.isEmpty() || !rep->summaryNotesEn.isEmpty())) {
            tip << "<hr><b>" + trBi("14-Tage Trend-Auffälligkeiten:", "14-Day Trend Anomalies:") + "</b>";
            const QStringList& notes = (LanguageManager::instance().currentLanguage() == AppLanguage::German)
                ? rep->summaryNotesDe : rep->summaryNotesEn;
            for (const QString& n : notes) {
                tip << "• " + n;
            }
        }
        tip << "<hr><i>" + trBi("Doppelklick für Detailansicht & Schichtprotokolle, Rechtsklick für Aktionen",
                                "Double-click for details & shift logs, right-click for actions") + "</i>";
        return tip.join("<br>");
    }

    return QVariant();
}

// ============================================================================
// AnlagenFilterProxyModel
// ============================================================================

AnlagenFilterProxyModel::AnlagenFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setSortRole(AnlagenTableModel::SortValueRole);
    setDynamicSortFilter(true);
}

void AnlagenFilterProxyModel::setSearchText(const QString& text) {
    beginFilterChange();
    m_searchText = text.trimmed();
    endFilterChange();
}

void AnlagenFilterProxyModel::setLinieFilter(const QString& linieId) {
    beginFilterChange();
    m_linieFilter = linieId.trimmed();
    endFilterChange();
}

void AnlagenFilterProxyModel::setTypFilter(int typEnumOrMinus1) {
    beginFilterChange();
    m_typFilter = typEnumOrMinus1;
    endFilterChange();
}

void AnlagenFilterProxyModel::setStatusFilter(int statusEnumOrMinus1) {
    beginFilterChange();
    m_statusFilter = statusEnumOrMinus1;
    endFilterChange();
}

void AnlagenFilterProxyModel::setMaxBaujahrFilter(int maxYearOr0) {
    beginFilterChange();
    m_maxBaujahr = maxYearOr0;
    endFilterChange();
}

void AnlagenFilterProxyModel::setMinBaujahrFilter(int minYearOr0) {
    beginFilterChange();
    m_minBaujahr = minYearOr0;
    endFilterChange();
}

void AnlagenFilterProxyModel::setOnlyConspicuous(bool only) {
    beginFilterChange();
    m_onlyConspicuous = only;
    endFilterChange();
}

void AnlagenFilterProxyModel::setOnlyDataWarnings(bool only) {
    beginFilterChange();
    m_onlyDataWarnings = only;
    endFilterChange();
}

void AnlagenFilterProxyModel::resetAllFilters() {
    beginFilterChange();
    m_searchText.clear();
    m_linieFilter.clear();
    m_typFilter = -1;
    m_statusFilter = -1;
    m_minBaujahr = 0;
    m_maxBaujahr = 0;
    m_onlyConspicuous = false;
    m_onlyDataWarnings = false;
    endFilterChange();
}

bool AnlagenFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    const QModelIndex idx0 = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx0.isValid()) return false;

    const QString anlageId = idx0.data(AnlagenTableModel::AnlageIdRole).toString();
    const QString linieId = idx0.data(AnlagenTableModel::LinieIdRole).toString();
    const QString bez = sourceModel()->index(sourceRow, AnlagenTableModel::ColBezeichnung, sourceParent).data().toString();
    const QString herst = sourceModel()->index(sourceRow, AnlagenTableModel::ColHerstellerModell, sourceParent).data().toString();

    // 1. Textsuche über anlage_id, bezeichnung, linie_id (und Hersteller/Modell)
    if (!m_searchText.isEmpty()) {
        const bool match = anlageId.contains(m_searchText, Qt::CaseInsensitive) ||
                           bez.contains(m_searchText, Qt::CaseInsensitive) ||
                           linieId.contains(m_searchText, Qt::CaseInsensitive) ||
                           herst.contains(m_searchText, Qt::CaseInsensitive);
        if (!match) return false;
    }

    // 2. Linien-Filter
    if (!m_linieFilter.isEmpty() && linieId != m_linieFilter) {
        return false;
    }

    // 3. Typ-Filter
    if (m_typFilter >= 0) {
        if (idx0.data(AnlagenTableModel::TypEnumRole).toInt() != m_typFilter) return false;
    }

    // 4. Status-Filter
    if (m_statusFilter >= 0) {
        if (idx0.data(AnlagenTableModel::StatusEnumRole).toInt() != m_statusFilter) return false;
    }

    // 5. Baujahr-Filter (z.B. Baujahr < 2018)
    const int baujahr = idx0.data(AnlagenTableModel::BaujahrRole).toInt();
    if (m_minBaujahr > 0 && baujahr < m_minBaujahr) return false;
    if (m_maxBaujahr > 0 && baujahr > m_maxBaujahr) return false;

    // 6. Nur auffällige Anlagen (14-Tage Trend)
    if (m_onlyConspicuous && !idx0.data(AnlagenTableModel::IsConspicuousRole).toBool()) {
        return false;
    }

    // 7. Nur Anlagen mit Stammdaten-Warnung
    if (m_onlyDataWarnings && !idx0.data(AnlagenTableModel::HasDataWarningRole).toBool()) {
        return false;
    }

    return true;
}

bool AnlagenFilterProxyModel::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const {
    const QVariant l = sourceLeft.data(AnlagenTableModel::SortValueRole);
    const QVariant r = sourceRight.data(AnlagenTableModel::SortValueRole);

    if (l.typeId() == QMetaType::Double || r.typeId() == QMetaType::Double) {
        return l.toDouble() < r.toDouble();
    }
    if (l.typeId() == QMetaType::Int || r.typeId() == QMetaType::Int) {
        return l.toInt() < r.toInt();
    }
    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

} // namespace RoboCell
