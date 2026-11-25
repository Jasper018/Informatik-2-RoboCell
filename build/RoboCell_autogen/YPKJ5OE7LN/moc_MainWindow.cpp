/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/mainwindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "slotOrdnerOeffnen",
        "",
        "slotNeuLaden",
        "slotSqliteLaden",
        "slotSqliteSpeichern",
        "slotCsvExportieren",
        "slotAusgewaehlteAnlageOeffnen",
        "slotAnlageBearbeiten",
        "slotWartungErfassen",
        "slotAnlageDoppelklick",
        "QModelIndex",
        "proxyIndex",
        "slotAnlageKontextmenue",
        "QPoint",
        "pos",
        "slotFilterGeaendert",
        "slotFilterZuruecksetzen",
        "slotSpaltenAnpassen",
        "slotFlagsTabZeigen",
        "slotEinstellungenOeffnen",
        "slotSpracheDeutsch",
        "slotSpracheEnglisch",
        "slotBerechtigungenZeigen",
        "slotBenutzerWechseln",
        "slotAbmelden",
        "slotOeeHilfe",
        "slotUeberProgramm",
        "slotParetoAktualisieren",
        "slotFlagFilterGeaendert",
        "slotFlagDoppelklick",
        "zeile",
        "spalte",
        "slotDatenGeaendert",
        "slotRollenAktualisieren"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'slotOrdnerOeffnen'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotNeuLaden'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotSqliteLaden'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotSqliteSpeichern'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotCsvExportieren'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotAusgewaehlteAnlageOeffnen'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotAnlageBearbeiten'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotWartungErfassen'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotAnlageDoppelklick'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Slot 'slotAnlageKontextmenue'
        QtMocHelpers::SlotData<void(const QPoint &)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 14, 15 },
        }}),
        // Slot 'slotFilterGeaendert'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotFilterZuruecksetzen'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotSpaltenAnpassen'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotFlagsTabZeigen'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotEinstellungenOeffnen'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotSpracheDeutsch'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotSpracheEnglisch'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotBerechtigungenZeigen'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotBenutzerWechseln'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotAbmelden'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotOeeHilfe'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotUeberProgramm'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotParetoAktualisieren'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotFlagFilterGeaendert'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotFlagDoppelklick'
        QtMocHelpers::SlotData<void(int, int)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 31 }, { QMetaType::Int, 32 },
        }}),
        // Slot 'slotDatenGeaendert'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'slotRollenAktualisieren'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->slotOrdnerOeffnen(); break;
        case 1: _t->slotNeuLaden(); break;
        case 2: _t->slotSqliteLaden(); break;
        case 3: _t->slotSqliteSpeichern(); break;
        case 4: _t->slotCsvExportieren(); break;
        case 5: _t->slotAusgewaehlteAnlageOeffnen(); break;
        case 6: _t->slotAnlageBearbeiten(); break;
        case 7: _t->slotWartungErfassen(); break;
        case 8: _t->slotAnlageDoppelklick((*reinterpret_cast<std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 9: _t->slotAnlageKontextmenue((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        case 10: _t->slotFilterGeaendert(); break;
        case 11: _t->slotFilterZuruecksetzen(); break;
        case 12: _t->slotSpaltenAnpassen(); break;
        case 13: _t->slotFlagsTabZeigen(); break;
        case 14: _t->slotEinstellungenOeffnen(); break;
        case 15: _t->slotSpracheDeutsch(); break;
        case 16: _t->slotSpracheEnglisch(); break;
        case 17: _t->slotBerechtigungenZeigen(); break;
        case 18: _t->slotBenutzerWechseln(); break;
        case 19: _t->slotAbmelden(); break;
        case 20: _t->slotOeeHilfe(); break;
        case 21: _t->slotUeberProgramm(); break;
        case 22: _t->slotParetoAktualisieren(); break;
        case 23: _t->slotFlagFilterGeaendert(); break;
        case 24: _t->slotFlagDoppelklick((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 25: _t->slotDatenGeaendert(); break;
        case 26: _t->slotRollenAktualisieren(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 27;
    }
    return _id;
}
QT_WARNING_POP
