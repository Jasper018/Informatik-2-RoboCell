#include "datentypen.h"
#include <QCoreApplication>

AnlagenTyp anlagenTypVonText(const QString& text)
{
    const QString t = text.trimmed().toLower();
    if (t == "cnc_beladung") return AnlagenTyp::CncBeladung;
    if (t == "schweissroboter") return AnlagenTyp::Schweissroboter;
    if (t == "montageroboter") return AnlagenTyp::Montageroboter;
    if (t == "schraubstation") return AnlagenTyp::Schraubstation;
    if (t == "klebestation") return AnlagenTyp::Klebestation;
    if (t == "pruefstation") return AnlagenTyp::Pruefstation;
    if (t == "laserbeschriftung") return AnlagenTyp::Laserbeschriftung;
    if (t == "palettierer") return AnlagenTyp::Palettierer;
    return AnlagenTyp::Unbekannt;
}

QString anlagenTypText(AnlagenTyp typ)
{
    switch (typ) {
    case AnlagenTyp::CncBeladung: return "cnc_beladung";
    case AnlagenTyp::Schweissroboter: return "schweissroboter";
    case AnlagenTyp::Montageroboter: return "montageroboter";
    case AnlagenTyp::Schraubstation: return "schraubstation";
    case AnlagenTyp::Klebestation: return "klebestation";
    case AnlagenTyp::Pruefstation: return "pruefstation";
    case AnlagenTyp::Laserbeschriftung: return "laserbeschriftung";
    case AnlagenTyp::Palettierer: return "palettierer";
    default: return "unbekannt";
    }
}

QString anlagenTypName(AnlagenTyp typ, const QString& roh)
{
    switch (typ) {
    case AnlagenTyp::CncBeladung: return QCoreApplication::translate("Stammdaten", "CNC-Beladung");
    case AnlagenTyp::Schweissroboter: return QCoreApplication::translate("Stammdaten", "Schweißroboter");
    case AnlagenTyp::Montageroboter: return QCoreApplication::translate("Stammdaten", "Montageroboter");
    case AnlagenTyp::Schraubstation: return QCoreApplication::translate("Stammdaten", "Schraubstation");
    case AnlagenTyp::Klebestation: return QCoreApplication::translate("Stammdaten", "Klebestation");
    case AnlagenTyp::Pruefstation: return QCoreApplication::translate("Stammdaten", "Prüfstation");
    case AnlagenTyp::Laserbeschriftung: return QCoreApplication::translate("Stammdaten", "Laserbeschriftung");
    case AnlagenTyp::Palettierer: return QCoreApplication::translate("Stammdaten", "Palettierer");
    default:
        return roh.isEmpty() ? QCoreApplication::translate("Stammdaten", "Unbekannt") : roh;
    }
}

AnlagenStatus anlagenStatusVonText(const QString& text)
{
    const QString t = text.trimmed().toLower();
    if (t == "in_betrieb") return AnlagenStatus::InBetrieb;
    if (t == "stoerung") return AnlagenStatus::Stoerung;
    if (t == "wartung") return AnlagenStatus::Wartung;
    if (t == "umbau") return AnlagenStatus::Umbau;
    return AnlagenStatus::Unbekannt;
}

QString anlagenStatusText(AnlagenStatus status)
{
    switch (status) {
    case AnlagenStatus::InBetrieb: return "in_betrieb";
    case AnlagenStatus::Stoerung: return "stoerung";
    case AnlagenStatus::Wartung: return "wartung";
    case AnlagenStatus::Umbau: return "umbau";
    default: return "unbekannt";
    }
}

QString anlagenStatusName(AnlagenStatus status, const QString& roh)
{
    switch (status) {
    case AnlagenStatus::InBetrieb: return QCoreApplication::translate("Stammdaten", "In Betrieb");
    case AnlagenStatus::Stoerung: return QCoreApplication::translate("Stammdaten", "Störung");
    case AnlagenStatus::Wartung: return QCoreApplication::translate("Stammdaten", "Wartung");
    case AnlagenStatus::Umbau: return QCoreApplication::translate("Stammdaten", "Umbau");
    default:
        return roh.isEmpty() ? QCoreApplication::translate("Stammdaten", "Unbekannt") : roh;
    }
}

ZustandTyp zustandTypVonText(const QString& text)
{
    const QString t = text.trimmed().toLower();
    if (t == "produktion") return ZustandTyp::Produktion;
    if (t == "ruesten") return ZustandTyp::Ruesten;
    if (t == "stoerung") return ZustandTyp::Stoerung;
    if (t == "materialmangel") return ZustandTyp::Materialmangel;
    if (t == "rueckstau") return ZustandTyp::Rueckstau;
    if (t == "wartung") return ZustandTyp::Wartung;
    if (t == "pause") return ZustandTyp::Pause;
    return ZustandTyp::Unbekannt;
}

QString zustandName(ZustandTyp typ)
{
    switch (typ) {
    case ZustandTyp::Produktion: return QCoreApplication::translate("Zustand", "Produktion");
    case ZustandTyp::Ruesten: return QCoreApplication::translate("Zustand", "Rüsten");
    case ZustandTyp::Stoerung: return QCoreApplication::translate("Zustand", "Störung");
    case ZustandTyp::Materialmangel: return QCoreApplication::translate("Zustand", "Materialmangel");
    case ZustandTyp::Rueckstau: return QCoreApplication::translate("Zustand", "Rückstau");
    case ZustandTyp::Wartung: return QCoreApplication::translate("Zustand", "Wartung");
    case ZustandTyp::Pause: return QCoreApplication::translate("Zustand", "Pause");
    default: return QCoreApplication::translate("Zustand", "Unbekannt");
    }
}

int Anlage::anzahlProbleme(Schweregrad grad) const
{
    int anzahl = 0;
    for (const Problem& p : probleme) {
        if (p.grad == grad) anzahl++;
    }
    return anzahl;
}

qint64 ZustandEintrag::dauerSekunden() const
{
    if (!beginn.isValid() || !ende.isValid()) return 0;
    const qint64 sekunden = beginn.secsTo(ende);
    return sekunden > 0 ? sekunden : 0;
}

int Schichtprotokoll::anzahlZustaende() const
{
    int anzahl = 0;
    for (const Schicht& s : schichten) {
        anzahl += s.zustaende.size();
    }
    return anzahl;
}
