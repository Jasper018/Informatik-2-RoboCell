#include "anlagenmodell.h"
#include "core/farben.h"
#include "io/datenverwaltung.h"
#include <QBrush>
#include <QFont>

AnlagenModell::AnlagenModell(Datenverwaltung* daten, QObject* parent)
    : QAbstractTableModel(parent)
    , m_daten(daten)
{
    cacheNeuBauen();
}

int AnlagenModell::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || !m_daten) return 0;
    return m_zeilen.size();
}

int AnlagenModell::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return SpaltenAnzahl;
}

void AnlagenModell::cacheNeuBauen()
{
    m_zeilen.clear();
    if (!m_daten) return;
    const QList<Anlage>& anlagen = m_daten->anlagen();
    m_zeilen.reserve(anlagen.size());
    for (const Anlage& a : anlagen) {
        ZeilenCache z;
        z.bericht = m_daten->trendbericht(a.anlageId);
        z.fehlerAnz = m_daten->anlagenProblemAnzahl(a.anlageId, Schweregrad::Fehler);
        z.warnAnz = m_daten->anlagenProblemAnzahl(a.anlageId, Schweregrad::Warnung);
        z.hatFlag = (z.fehlerAnz + z.warnAnz) > 0;
        z.herstellerModell = a.hersteller + " " + a.modell;
        z.oeeText = (z.bericht.gueltigeTage > 0)
            ? QString("%1 %").arg(z.bericht.mittelOee * 100.0, 0, 'f', 1)
            : QString("-");
        z.stoerzeitText = QString("%1").arg(z.bericht.stoerzeitSek / 60.0, 0, 'f', 0);
        m_zeilen.append(z);
    }
}

void AnlagenModell::aktualisieren()
{
    beginResetModel();
    cacheNeuBauen();
    endResetModel();
}

void AnlagenModell::spracheAktualisieren()
{
    emit headerDataChanged(Qt::Horizontal, 0, SpaltenAnzahl - 1);
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, SpaltenAnzahl - 1));
    }
}

void AnlagenModell::farbenAktualisieren()
{
    if (rowCount() > 0) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, SpaltenAnzahl - 1),
                         {Qt::BackgroundRole, Qt::ForegroundRole});
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
    if (zeile < 0 || zeile >= m_zeilen.size() || zeile >= m_daten->anlagen().size()) {
        return QVariant();
    }

    const Anlage& a = m_daten->anlagen().at(zeile);
    const ZeilenCache& z = m_zeilen.at(zeile);

    if (role == AnlageIdRolle) return a.anlageId;
    if (role == LinieIdRolle) return a.linieId;
    if (role == TypRolle) return int(a.typ);
    if (role == StatusRolle) return int(a.status);
    if (role == BaujahrRolle) return a.baujahr;
    if (role == AuffaelligRolle) return z.bericht.auffaellig;
    if (role == HatFlagRolle) return z.hatFlag;

    if (role == SortierWertRolle) {
        switch (spalte) {
        case SpAnlageId: return a.anlageId;
        case SpLinieId: return a.linieId;
        case SpPosition: return a.position;
        case SpBezeichnung: return a.bezeichnung;
        case SpTyp: return anlagenTypName(a.typ, a.typRoh);
        case SpHerstellerModell: return z.herstellerModell;
        case SpBaujahr: return a.baujahr;
        case SpSollTakt: return a.sollTaktzeitS;
        case SpStatus: return int(a.status);
        case SpMittelOee: return z.bericht.gueltigeTage > 0 ? z.bericht.mittelOee : -1.0;
        case SpStoerzeitMin: return z.bericht.stoerzeitSek / 60.0;
        case SpFlags: return z.fehlerAnz * 100 + z.warnAnz * 10 + (z.bericht.auffaellig ? 1 : 0);
        }
    }

    if (role == Qt::DisplayRole) {
        switch (spalte) {
        case SpAnlageId: return a.anlageId;
        case SpLinieId: return a.linieId;
        case SpPosition: return a.position;
        case SpBezeichnung: return a.bezeichnung;
        case SpTyp: return anlagenTypName(a.typ, a.typRoh);
        case SpHerstellerModell: return z.herstellerModell;
        case SpBaujahr: return a.baujahr;
        case SpSollTakt: return QString::number(a.sollTaktzeitS, 'f', 1);
        case SpStatus: return anlagenStatusName(a.status, a.statusRoh);
        case SpMittelOee: return z.oeeText;
        case SpStoerzeitMin: return z.stoerzeitText;
        case SpFlags: {
            QStringList teile;
            if (z.fehlerAnz > 0) teile.append(tr("Fehler (%1)").arg(z.fehlerAnz));
            if (z.warnAnz > 0) teile.append(tr("Warnung (%1)").arg(z.warnAnz));
            if (z.bericht.auffaellig) teile.append(tr("Auffällig"));
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

    // Alle Hintergrund-/Vordergrundpaare stammen aus der aktiven Palette (>= 8:1 Kontrast)
    if (role == Qt::BackgroundRole) {
        if (spalte == SpStatus) {
            switch (a.status) {
            case AnlagenStatus::InBetrieb: return QBrush(Farben::okHintergrund());
            case AnlagenStatus::Stoerung: return QBrush(Farben::fehlerHintergrund());
            case AnlagenStatus::Wartung:
            case AnlagenStatus::Umbau: return QBrush(Farben::warnHintergrund());
            default: return QBrush(Farben::fehlerHintergrund());
            }
        }
        if (spalte == SpMittelOee && z.bericht.gueltigeTage > 0) {
            if (z.bericht.mittelOee >= 0.80) return QBrush(Farben::okHintergrund());
            if (z.bericht.mittelOee >= 0.70) return QBrush(Farben::warnHintergrund());
            return QBrush(Farben::fehlerHintergrund());
        }
        if (spalte == SpFlags) {
            if (z.fehlerAnz > 0) return QBrush(Farben::fehlerHintergrund());
            if (z.warnAnz > 0 || z.bericht.auffaellig) return QBrush(Farben::warnHintergrund());
        }
    }

    if (role == Qt::ForegroundRole) {
        if (spalte == SpStatus) {
            switch (a.status) {
            case AnlagenStatus::InBetrieb: return QBrush(Farben::okText());
            case AnlagenStatus::Stoerung: return QBrush(Farben::fehlerText());
            case AnlagenStatus::Wartung:
            case AnlagenStatus::Umbau: return QBrush(Farben::warnText());
            default: return QBrush(Farben::fehlerText());
            }
        }
        if (spalte == SpMittelOee && z.bericht.gueltigeTage > 0) {
            if (z.bericht.mittelOee >= 0.80) return QBrush(Farben::okText());
            if (z.bericht.mittelOee >= 0.70) return QBrush(Farben::warnText());
            return QBrush(Farben::fehlerText());
        }
        if (spalte == SpFlags) {
            if (z.fehlerAnz > 0) return QBrush(Farben::fehlerText());
            if (z.warnAnz > 0 || z.bericht.auffaellig) return QBrush(Farben::warnText());
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
        const QStringList trendHinweise = z.bericht.hinweise();
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
