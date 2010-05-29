/****************************************************************************
** Meta object code from reading C++ file 'skin.h'
**
** Created
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "skin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'skin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SkinConfigWidget[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   12, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // slots: signature, parameters, type, tag, flags
      34,   18,   17,   17, 0x0a,
      62,   56,   17,   17, 0x0a,
      80,   17,   17,   17, 0x0a,
     100,   17,   17,   17, 0x0a,
     115,   17,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SkinConfigWidget[] = {
    "SkinConfigWidget\0\0skin,stylesheet\0"
    "load(QString,QString)\0index\0"
    "skinSelected(int)\0stylesheetChanged()\0"
    "deleteCustom()\0saveCustom()\0"
};

const QMetaObject SkinConfigWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SkinConfigWidget,
      qt_meta_data_SkinConfigWidget, 0 }
};

const QMetaObject *SkinConfigWidget::metaObject() const
{
    return &staticMetaObject;
}

void *SkinConfigWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SkinConfigWidget))
        return static_cast<void*>(const_cast< SkinConfigWidget*>(this));
    if (!strcmp(_clname, "Ui::SkinConfigWidget"))
        return static_cast< Ui::SkinConfigWidget*>(const_cast< SkinConfigWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int SkinConfigWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: load((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 1: skinSelected((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: stylesheetChanged(); break;
        case 3: deleteCustom(); break;
        case 4: saveCustom(); break;
        default: ;
        }
        _id -= 5;
    }
    return _id;
}
static const uint qt_meta_data_SkinPlugin[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

       0        // eod
};

static const char qt_meta_stringdata_SkinPlugin[] = {
    "SkinPlugin\0"
};

const QMetaObject SkinPlugin::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SkinPlugin,
      qt_meta_data_SkinPlugin, 0 }
};

const QMetaObject *SkinPlugin::metaObject() const
{
    return &staticMetaObject;
}

void *SkinPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SkinPlugin))
        return static_cast<void*>(const_cast< SkinPlugin*>(this));
    if (!strcmp(_clname, "KLFPluginGenericInterface"))
        return static_cast< KLFPluginGenericInterface*>(const_cast< SkinPlugin*>(this));
    if (!strcmp(_clname, "org.klatexformula.KLatexFormula.Plugin.GenericInterface/1.0"))
        return static_cast< KLFPluginGenericInterface*>(const_cast< SkinPlugin*>(this));
    return QObject::qt_metacast(_clname);
}

int SkinPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
