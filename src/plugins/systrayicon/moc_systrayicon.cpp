/****************************************************************************
** Meta object code from reading C++ file 'systrayicon.h'
**
** Created
**      by: The Qt Meta Object Compiler version 61 (Qt 4.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "systrayicon.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'systrayicon.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 61
#error "This file was generated using the moc from 4.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SysTrayIconConfigWidget[] = {

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

static const char qt_meta_stringdata_SysTrayIconConfigWidget[] = {
    "SysTrayIconConfigWidget\0"
};

const QMetaObject SysTrayIconConfigWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SysTrayIconConfigWidget,
      qt_meta_data_SysTrayIconConfigWidget, 0 }
};

const QMetaObject *SysTrayIconConfigWidget::metaObject() const
{
    return &staticMetaObject;
}

void *SysTrayIconConfigWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SysTrayIconConfigWidget))
        return static_cast<void*>(const_cast< SysTrayIconConfigWidget*>(this));
    if (!strcmp(_clname, "Ui::SysTrayIconConfigWidget"))
        return static_cast< Ui::SysTrayIconConfigWidget*>(const_cast< SysTrayIconConfigWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int SysTrayIconConfigWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_SysTrayMainIconifyButtons[] = {

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

static const char qt_meta_stringdata_SysTrayMainIconifyButtons[] = {
    "SysTrayMainIconifyButtons\0"
};

const QMetaObject SysTrayMainIconifyButtons::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SysTrayMainIconifyButtons,
      qt_meta_data_SysTrayMainIconifyButtons, 0 }
};

const QMetaObject *SysTrayMainIconifyButtons::metaObject() const
{
    return &staticMetaObject;
}

void *SysTrayMainIconifyButtons::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SysTrayMainIconifyButtons))
        return static_cast<void*>(const_cast< SysTrayMainIconifyButtons*>(this));
    if (!strcmp(_clname, "Ui::SysTrayMainIconifyButtons"))
        return static_cast< Ui::SysTrayMainIconifyButtons*>(const_cast< SysTrayMainIconifyButtons*>(this));
    return QWidget::qt_metacast(_clname);
}

int SysTrayMainIconifyButtons::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_SysTrayIconPlugin[] = {

 // content:
       2,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   12, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors

 // signals: signature, parameters, type, tag, flags
      24,   19,   18,   18, 0x05,

 // slots: signature, parameters, type, tag, flags
      46,   18,   18,   18, 0x0a,
      56,   18,   18,   18, 0x0a,
      67,   18,   18,   18, 0x0a,
     128,  123,   18,   18, 0x0a,
     165,   18,   18,   18, 0x2a,
     186,   18,   18,   18, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SysTrayIconPlugin[] = {
    "SysTrayIconPlugin\0\0text\0setLatexText(QString)\0"
    "restore()\0minimize()\0"
    "slotSysTrayActivated(QSystemTrayIcon::ActivationReason)\0"
    "mode\0latexFromClipboard(QClipboard::Mode)\0"
    "latexFromClipboard()\0latexFromClipboardSelection()\0"
};

const QMetaObject SysTrayIconPlugin::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_SysTrayIconPlugin,
      qt_meta_data_SysTrayIconPlugin, 0 }
};

const QMetaObject *SysTrayIconPlugin::metaObject() const
{
    return &staticMetaObject;
}

void *SysTrayIconPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SysTrayIconPlugin))
        return static_cast<void*>(const_cast< SysTrayIconPlugin*>(this));
    if (!strcmp(_clname, "KLFPluginGenericInterface"))
        return static_cast< KLFPluginGenericInterface*>(const_cast< SysTrayIconPlugin*>(this));
    if (!strcmp(_clname, "org.klatexformula.KLatexFormula.Plugin.GenericInterface/1.0"))
        return static_cast< KLFPluginGenericInterface*>(const_cast< SysTrayIconPlugin*>(this));
    return QObject::qt_metacast(_clname);
}

int SysTrayIconPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: setLatexText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: restore(); break;
        case 2: minimize(); break;
        case 3: slotSysTrayActivated((*reinterpret_cast< QSystemTrayIcon::ActivationReason(*)>(_a[1]))); break;
        case 4: latexFromClipboard((*reinterpret_cast< QClipboard::Mode(*)>(_a[1]))); break;
        case 5: latexFromClipboard(); break;
        case 6: latexFromClipboardSelection(); break;
        default: ;
        }
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void SysTrayIconPlugin::setLatexText(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
