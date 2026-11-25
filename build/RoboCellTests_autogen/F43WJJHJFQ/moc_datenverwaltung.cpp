/****************************************************************************
** Meta object code from reading C++ file 'datenverwaltung.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/io/datenverwaltung.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'datenverwaltung.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15DatenverwaltungE_t {};
} // unnamed namespace

template <> constexpr inline auto Datenverwaltung::qt_create_metaobjectdata<qt_meta_tag_ZN15DatenverwaltungE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Datenverwaltung",
        "pruefFortschritt",
        "",
        "aktuell",
        "gesamt",
        "datensatzGeladen",
        "anlageGeaendert",
        "anlageId",
        "wartungHinzugefuegt"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'pruefFortschritt'
        QtMocHelpers::SignalData<void(int, int)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 4 },
        }}),
        // Signal 'datensatzGeladen'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'anlageGeaendert'
        QtMocHelpers::SignalData<void(const QString &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
        // Signal 'wartungHinzugefuegt'
        QtMocHelpers::SignalData<void(const QString &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 7 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Datenverwaltung, qt_meta_tag_ZN15DatenverwaltungE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Datenverwaltung::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15DatenverwaltungE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15DatenverwaltungE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15DatenverwaltungE_t>.metaTypes,
    nullptr
} };

void Datenverwaltung::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Datenverwaltung *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pruefFortschritt((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 1: _t->datensatzGeladen(); break;
        case 2: _t->anlageGeaendert((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->wartungHinzugefuegt((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Datenverwaltung::*)(int , int )>(_a, &Datenverwaltung::pruefFortschritt, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Datenverwaltung::*)()>(_a, &Datenverwaltung::datensatzGeladen, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Datenverwaltung::*)(const QString & )>(_a, &Datenverwaltung::anlageGeaendert, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Datenverwaltung::*)(const QString & )>(_a, &Datenverwaltung::wartungHinzugefuegt, 3))
            return;
    }
}

const QMetaObject *Datenverwaltung::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Datenverwaltung::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15DatenverwaltungE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Datenverwaltung::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void Datenverwaltung::pruefFortschritt(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void Datenverwaltung::datensatzGeladen()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Datenverwaltung::anlageGeaendert(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void Datenverwaltung::wartungHinzugefuegt(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
