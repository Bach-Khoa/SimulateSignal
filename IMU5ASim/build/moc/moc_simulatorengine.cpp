/****************************************************************************
** Meta object code from reading C++ file 'simulatorengine.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../simulatorengine.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'simulatorengine.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN15SimulatorEngineE_t {};
} // unnamed namespace

template <> constexpr inline auto SimulatorEngine::qt_create_metaobjectdata<qt_meta_tag_ZN15SimulatorEngineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SimulatorEngine",
        "statsUpdated",
        "",
        "index",
        "total",
        "sentTotal",
        "packetReady",
        "packet",
        "ImuFrame",
        "frame",
        "errorOccurred",
        "message",
        "finished"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'statsUpdated'
        QtMocHelpers::SignalData<void(int, int, qint64)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Int, 4 }, { QMetaType::LongLong, 5 },
        }}),
        // Signal 'packetReady'
        QtMocHelpers::SignalData<void(QByteArray, ImuFrame)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QByteArray, 7 }, { 0x80000000 | 8, 9 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(QString)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 11 },
        }}),
        // Signal 'finished'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SimulatorEngine, qt_meta_tag_ZN15SimulatorEngineE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SimulatorEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15SimulatorEngineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15SimulatorEngineE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN15SimulatorEngineE_t>.metaTypes,
    nullptr
} };

void SimulatorEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SimulatorEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->statsUpdated((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<qint64>>(_a[3]))); break;
        case 1: _t->packetReady((*reinterpret_cast<std::add_pointer_t<QByteArray>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<ImuFrame>>(_a[2]))); break;
        case 2: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->finished(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SimulatorEngine::*)(int , int , qint64 )>(_a, &SimulatorEngine::statsUpdated, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SimulatorEngine::*)(QByteArray , ImuFrame )>(_a, &SimulatorEngine::packetReady, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SimulatorEngine::*)(QString )>(_a, &SimulatorEngine::errorOccurred, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SimulatorEngine::*)()>(_a, &SimulatorEngine::finished, 3))
            return;
    }
}

const QMetaObject *SimulatorEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SimulatorEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN15SimulatorEngineE_t>.strings))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int SimulatorEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
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
void SimulatorEngine::statsUpdated(int _t1, int _t2, qint64 _t3)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2, _t3);
}

// SIGNAL 1
void SimulatorEngine::packetReady(QByteArray _t1, ImuFrame _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void SimulatorEngine::errorOccurred(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void SimulatorEngine::finished()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
