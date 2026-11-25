#include "anlagenmodell.h"
#include "core/farben.h"
#include "io/datenverwaltung.h"
#include <QBrush>
#include <QFont>

AnlagenModell::AnlagenModell(Datenverwaltung* daten, QObject* parent)
    : QAbstractTableModel(parent)
    , m_daten(daten)
{
}

int AnlagenModell::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_daten) return 0;
    return m_daten->anlagen().size();
}

int AnlagenModell::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return SpaltenAnzahl;
}

void AnlagenModell::aktualisieren()
{
    beginResetModel();
    endResetModel();
}

void AnlagenModell::spracheAktualisieren()
{
    emit headerDataChanged(Qt::Horizontal, 0, SpaltenAnzahl - 1);
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, SpaltenAnzahl - 1));
    }
}

QVariant AnlagenModell::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();

    switch (section) {
    case SpAnlageId: return tr("Anlage-ID");
    case SpLinieId: return tr("Linie");
    case SpPosition: return tr("Pos.");
    case SpBezeichnung: return tr("Bezeichnung");
    case SpTyp: return tr("Typ");
    case SpHerstellerModell: return tr("Hersteller / Modell");
    case SpBaujahr: return tr("Baujahr");
    case SpSollTakt: return tr("Soll-Takt [s]");
    case SpStatus: return tr("Status");
    case SpMittelOee: return tr("Ø OEE (14 Tage)");
    case SpStoerzeitMin: return tr("Störzeit [min]");
    case SpFlags: return tr("Prüfung / Flags");
    }
    return QVariant();
}

QVariant AnlagenModell::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !m_daten) return QVariant();
    const int zeile = index.row();
    const int spalte = index.column();
    if (zeile < 0 || zeile >= m_daten->anlagen().size()) return QVariant();

    const Anlage& a = m_daten->anlagen().at(zeile);
    const Trendbericht bericht = m_daten->trendbericht(a.anlageId);
    const int fehlerAnz = m_daten->anlagenProblemAnzahl(a.anlageId, Schweregrad::Fehler);
    const int warnAnz = m_daten->anlagenProblemAnzahl(a.anlageId, Schweregrad::Warnung);
    const bool hatFlag = (fehlerAnz + warnAnz) > 0;

    if (role == AnlageIdRolle) return a.anlageId;
    if (role == LinieIdRolle) return a.linieId;
    if (role == TypRolle) return int(a.typ);
    if (role == StatusRolle) return int(a.status);
    if (role == BaujahrRolle) return a.baujahr;
    if (role == AuffaelligRolle) return bericht.auffaellig;
    if (role == HatFlagRolle) return hatFlag;

    if (role == SortierWertRolle) {
        switch (spalte) {
        case SpAnlageId: return a.anlageId;
        case SpLinieId: return a.linieId;
        case SpPosition: return a.position;
        case SpBezeichnung: return a.bezeichnung;
        case SpTyp: return anlagenTypName(a.typ, a.typRoh);
        case SpHerstellerModell: return a.hersteller + " " + a.modell;
        case SpBaujahr: return a.baujahr;
        case SpSollTakt: return a.sollTaktzeitS;
        case SpStatus: return int(a.status);
        case SpMittelOee: return bericht.gueltigeTage > 0 ? bericht.mittelOee : -1.0;
        case SpStoerzeitMin: return bericht.stoerzeitSek / 60.0;
        case SpFlags: return fehlerAnz * 100 + warnAnz * 10 + (bericht.auffaellig ? 1 : 0);
        }
    }

    if (role == Qt::DisplayRole) {
        switch (spalte) {
        case SpAnlageId: return a.anlageId;
        case SpLinieId: return a.linieId;
        case SpPosition: return a.position;
        case SpBezeichnung: return a.bezeichnung;
        case SpTyp: return anlagenTypName(a.typ, a.typRoh);
        case SpHerstellerModell: return a.hersteller + " " + a.modell;
        case SpBaujahr: return a.baujahr;
        case SpSollTakt: return QString::number(a.sollTaktzeitS, 'f', 1);
        case SpStatus: return anlagenStatusName(a.status, a.statusRoh);
        case SpMittelOee:
            return (bericht.gueltigeTage > 0)
                ? QString("%1 %").arg(bericht.mittelOee * 100.0, 0, 'f', 1)
                : QString("-");
        case SpStoerzeitMin:
            return QString("%1").arg(bericht.stoerzeitSek / 60.0, 0, 'f', 0);
        case SpFlags: {
            QStringList teile;
            if (fehlerAnz > 0) teile.append(tr("Fehler (%1)").arg(fehlerAnz));
            if (warnAnz > 0) teile.append(tr("Warnung (%1)").arg(warnAnz));
            if (bericht.auffaellig) teile.append(tr("Auffällig"));
            if (a.bearbeitet) teile.append(tr("Bearbeitet"));
            return teile.isEmpty() ? tr("OK") : teile.join(", ");
        }
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (spalte == SpPosition || spalte == SpBaujahr || spalte == SpSollTakt
            || spalte == SpMittelOee || spalte == SpStoerzeitMin) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        }
        if (spalte == SpAnlageId || spalte == SpLinieId || spalte == SpStatus) {
            return int(Qt::AlignCenter);
        }
    }

    // Alle Hintergrund-/Vordergrundpaare stammen aus Farben (>= 11,5:1 Kontrast)
    if (role == Qt::BackgroundRole) {
        if (spalte == SpStatus) {
            switch (a.status) {
            case AnlagenStatus::InBetrieb: return QBrush(Farben::OK_HINTERGRUND);
            case AnlagenStatus::Stoerung: return QBrush(Farben::FEHLER_HINTERGRUND);
            case AnlagenStatus::Wartung: return QBrush(Farben::WARN_HINTERGRUND);
            case AnlagenStatus::Umbau: return QBrush(Farben::INFO_HINTERGRUND);
            default: return QBrush(Farben::FEHLER_HINTERGRUND);
            }
        }
        if (spalte == SpMittelOee && bericht.gueltigeTage > 0) {
            if (bericht.mittelOee >= 0.80) return QBrush(Farben::OK_HINTERGRUND);
            if (bericht.mittelOee >= 0.70) return QBrush(Farben::WARN_HINTERGRUND);
            return QBrush(Farben::FEHLER_HINTERGRUND);
        }
        if (spalte == SpFlags) {
            if (fehlerAnz > 0) return QBrush(Farben::FEHLER_HINTERGRUND);
            if (warnAnz > 0 || bericht.auffaellig) return QBrush(Farben::WARN_HINTERGRUND);
            if (a.bearbeitet) return QBrush(Farben::INFO_HINTERGRUND);
        }
    }

    if (role == Qt::ForegroundRole) {
        if (spalte == SpStatus) {
            switch (a.status) {
            case AnlagenStatus::InBetrieb: return QBrush(Farben::OK_TEXT);
            case AnlagenStatus::Stoerung: return QBrush(Farben::FEHLER_TEXT);
            case AnlagenStatus::Wartung: return QBrush(Farben::WARN_TEXT);
            case AnlagenStatus::Umbau: return QBrush(Farben::INFO_TEXT);
            default: return QBrush(Farben::FEHLER_TEXT);
            }
        }
        if (spalte == SpMittelOee && bericht.gueltigeTage > 0) {
            if (bericht.mittelOee >= 0.80) return QBrush(Farben::OK_TEXT);
            if (bericht.mittelOee >= 0.70) return QBrush(Farben::WARN_TEXT);
            return QBrush(Farben::FEHLER_TEXT);
        }
        if (spalte == SpFlags) {
            if (fehlerAnz > 0) return QBrush(Farben::FEHLER_TEXT);
            if (warnAnz > 0 || bericht.auffaellig) return QBrush(Farben::WARN_TEXT);
            if (a.bearbeitet) return QBrush(Farben::INFO_TEXT);
        }
    }

    if (role == Qt::FontRole) {
        if (spalte == SpAnlageId || spalte == SpStatus || spalte == SpMittelOee) {
            QFont f;
            f.setBold(true);
            return f;
        }
    }

    if (role == Qt::ToolTipRole) {
        QStringList zeilen;
        zeilen.append(QString("%1 - %2").arg(a.anlageId, a.bezeichnung));
        zeilen.append(tr("Linie: %1, Position: %2").arg(a.linieId).arg(a.position));
        zeilen.append(tr("Hersteller/Modell: %1 %2 (%3 Achsen, %4 kg)")
                          .arg(a.hersteller, a.modell).arg(a.achsen).arg(a.traglastKg, 0, 'f', 1));
        const QList<Problem> probs = m_daten->anlagenProbleme(a.anlageId);
        if (!probs.isEmpty()) {
            zeilen.append(tr("Erkannte Flags (%1):").arg(probs.size()));
            const int limit = qMin(4, probs.size());
            for (int i = 0; i < limit; i++) {
                zeilen.append(QString(" - [%1] %2").arg(probs[i].gradText(), probs[i].text()));
            }
            if (probs.size() > limit) {
                zeilen.append(tr(" - ... und %1 weitere").arg(probs.size() - limit));
            }
        }
        const QStringList trendHinweise = bericht.hinweise();
        if (!trendHinweise.isEmpty()) {
            zeilen.append(tr("14-Tage-Trend:"));
            for (const QString& h : trendHinweise) {
                zeilen.append(" - " + h);
            }
        }
        return zeilen.join("\n");
    }

    return QVariant();
}

// ===========================================================================
// Filtermodell
// ===========================================================================
AnlagenFilterModell::AnlagenFilterModell(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setSortRole(AnlagenModell::SortierWertRolle);
    setDynamicSortFilter(true);
}

void AnlagenFilterModell::setSuchtext(const QString& text)
{
    beginFilterChange();
    m_suchtext = text.trimmed();
    endFilterChange();
}

void AnlagenFilterModell::setLinieFilter(const QString& linieId)
{
    beginFilterChange();
    m_linie = linieId.trimmed();
    endFilterChange();
}

void AnlagenFilterModell::setTypFilter(int typOderMinus1)
{
    beginFilterChange();
    m_typ = typOderMinus1;
    endFilterChange();
}

void AnlagenFilterModell::setStatusFilter(int statusOderMinus1)
{
    beginFilterChange();
    m_status = statusOderMinus1;
    endFilterChange();
}

void AnlagenFilterModell::setMaxBaujahr(int jahrOder0)
{
    beginFilterChange();
    m_maxBaujahr = jahrOder0;
    endFilterChange();
}

void AnlagenFilterModell::setNurAuffaellige(bool nur)
{
    beginFilterChange();
    m_nurAuffaellig = nur;
    endFilterChange();
}

void AnlagenFilterModell::setNurMitFlag(bool nur)
{
    beginFilterChange();
    m_nurMitFlag = nur;
    endFilterChange();
}

void AnlagenFilterModell::filterZuruecksetzen()
{
    beginFilterChange();
    m_suchtext.clear();
    m_linie.clear();
    m_typ = -1;
    m_status = -1;
    m_maxBaujahr = 0;
    m_nurAuffaellig = false;
    m_nurMitFlag = false;
    endFilterChange();
}

bool AnlagenFilterModell::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx.isValid()) return false;

    const QString id = idx.data(AnlagenModell::AnlageIdRolle).toString();
    const QString linie = idx.data(AnlagenModell::LinieIdRolle).toString();
    const QString bez = sourceModel()->index(sourceRow, AnlagenModell::SpBezeichnung, sourceParent).data().toString();
    const QString herst = sourceModel()->index(sourceRow, AnlagenModell::SpHerstellerModell, sourceParent).data().toString();

    if (!m_suchtext.isEmpty()) {
        const bool treffer = id.contains(m_suchtext, Qt::CaseInsensitive)
                             || bez.contains(m_suchtext, Qt::CaseInsensitive)
                             || linie.contains(m_suchtext, Qt::CaseInsensitive)
                             || herst.contains(m_suchtext, Qt::CaseInsensitive);
        if (!treffer) return false;
    }

    if (!m_linie.isEmpty() && linie != m_linie) return false;
    if (m_typ >= 0 && idx.data(AnlagenModell::TypRolle).toInt() != m_typ) return false;
    if (m_status >= 0 && idx.data(AnlagenModell::StatusRolle).toInt() != m_status) return false;

    const int baujahr = idx.data(AnlagenModell::BaujahrRolle).toInt();
    if (m_maxBaujahr > 0 && baujahr > m_maxBaujahr) return false;
    if (m_nurAuffaellig && !idx.data(AnlagenModell::AuffaelligRolle).toBool()) return false;
    if (m_nurMitFlag && !idx.data(AnlagenModell::HatFlagRolle).toBool()) return false;

    return true;
}

bool AnlagenFilterModell::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const QVariant l = left.data(AnlagenModell::SortierWertRolle);
    const QVariant r = right.data(AnlagenModell::SortierWertRolle);
    if (l.typeId() == QMetaType::Double || r.typeId() == QMetaType::Double) {
        return l.toDouble() < r.toDouble();
    }
    if (l.typeId() == QMetaType::Int || r.typeId() == QMetaType::Int) {
        return l.toInt() < r.toInt();
    }
    return QSortFilterProxyModel::lessThan(left, right);
}
