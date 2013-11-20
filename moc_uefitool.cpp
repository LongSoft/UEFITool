/****************************************************************************
** Meta object code from reading C++ file 'uefitool.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "uefitool.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'uefitool.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_UEFITool[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x08,
      17,    9,    9,    9, 0x08,
      33,    9,    9,    9, 0x08,
      57,   49,    9,    9, 0x08,
      81,    9,    9,    9, 0x08,
     109,  104,    9,    9, 0x08,
     124,    9,    9,    9, 0x08,
     138,    9,    9,    9, 0x08,
     152,    9,    9,    9, 0x08,
     174,  104,    9,    9, 0x08,
     188,    9,    9,    9, 0x08,
     201,    9,    9,    9, 0x08,
     216,    9,    9,    9, 0x08,
     230,    9,    9,    9, 0x08,
     240,    9,    9,    9, 0x08,
     249,    9,    9,    9, 0x08,
     259,    9,    9,    9, 0x08,
     274,    9,    9,    9, 0x08,
     290,    9,    9,    9, 0x08,
     306,    9,    9,    9, 0x08,
     321,    9,    9,    9, 0x08,
     329,    9,    9,    9, 0x08,
     344,  339,    9,    9, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_UEFITool[] = {
    "UEFITool\0\0init()\0openImageFile()\0"
    "saveImageFile()\0current\0populateUi(QModelIndex)\0"
    "resizeTreeViewColums()\0mode\0extract(UINT8)\0"
    "extractAsIs()\0extractBody()\0"
    "extractUncompressed()\0insert(UINT8)\0"
    "insertInto()\0insertBefore()\0insertAfter()\0"
    "replace()\0remove()\0rebuild()\0"
    "changeToNone()\0changeToEfi11()\0"
    "changeToTiano()\0changeToLzma()\0about()\0"
    "aboutQt()\0item\0scrollTreeView(QListWidgetItem*)\0"
};

void UEFITool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        UEFITool *_t = static_cast<UEFITool *>(_o);
        switch (_id) {
        case 0: _t->init(); break;
        case 1: _t->openImageFile(); break;
        case 2: _t->saveImageFile(); break;
        case 3: _t->populateUi((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 4: _t->resizeTreeViewColums(); break;
        case 5: _t->extract((*reinterpret_cast< const UINT8(*)>(_a[1]))); break;
        case 6: _t->extractAsIs(); break;
        case 7: _t->extractBody(); break;
        case 8: _t->extractUncompressed(); break;
        case 9: _t->insert((*reinterpret_cast< const UINT8(*)>(_a[1]))); break;
        case 10: _t->insertInto(); break;
        case 11: _t->insertBefore(); break;
        case 12: _t->insertAfter(); break;
        case 13: _t->replace(); break;
        case 14: _t->remove(); break;
        case 15: _t->rebuild(); break;
        case 16: _t->changeToNone(); break;
        case 17: _t->changeToEfi11(); break;
        case 18: _t->changeToTiano(); break;
        case 19: _t->changeToLzma(); break;
        case 20: _t->about(); break;
        case 21: _t->aboutQt(); break;
        case 22: _t->scrollTreeView((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData UEFITool::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject UEFITool::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_UEFITool,
      qt_meta_data_UEFITool, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &UEFITool::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *UEFITool::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *UEFITool::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_UEFITool))
        return static_cast<void*>(const_cast< UEFITool*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int UEFITool::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
