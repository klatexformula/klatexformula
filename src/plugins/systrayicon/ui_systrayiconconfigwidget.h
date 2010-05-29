/********************************************************************************
** Form generated from reading ui file 'systrayiconconfigwidget.ui'
**
** Created
**      by: Qt User Interface Compiler version 4.5.3
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_SYSTRAYICONCONFIGWIDGET_H
#define UI_SYSTRAYICONCONFIGWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SysTrayIconConfigWidget
{
public:
    QGridLayout *gridLayout;
    QCheckBox *chkSysTrayOn;
    QSpacerItem *verticalSpacer;
    QCheckBox *chkRestoreOnHover;
    QCheckBox *chkReplaceQuitButton;

    void setupUi(QWidget *SysTrayIconConfigWidget)
    {
        if (SysTrayIconConfigWidget->objectName().isEmpty())
            SysTrayIconConfigWidget->setObjectName(QString::fromUtf8("SysTrayIconConfigWidget"));
        SysTrayIconConfigWidget->resize(516, 371);
        gridLayout = new QGridLayout(SysTrayIconConfigWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        chkSysTrayOn = new QCheckBox(SysTrayIconConfigWidget);
        chkSysTrayOn->setObjectName(QString::fromUtf8("chkSysTrayOn"));

        gridLayout->addWidget(chkSysTrayOn, 0, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 3, 0, 1, 1);

        chkRestoreOnHover = new QCheckBox(SysTrayIconConfigWidget);
        chkRestoreOnHover->setObjectName(QString::fromUtf8("chkRestoreOnHover"));

        gridLayout->addWidget(chkRestoreOnHover, 1, 0, 1, 1);

        chkReplaceQuitButton = new QCheckBox(SysTrayIconConfigWidget);
        chkReplaceQuitButton->setObjectName(QString::fromUtf8("chkReplaceQuitButton"));

        gridLayout->addWidget(chkReplaceQuitButton, 2, 0, 1, 1);


        retranslateUi(SysTrayIconConfigWidget);
        QObject::connect(chkSysTrayOn, SIGNAL(toggled(bool)), chkRestoreOnHover, SLOT(setEnabled(bool)));
        QObject::connect(chkSysTrayOn, SIGNAL(toggled(bool)), chkReplaceQuitButton, SLOT(setEnabled(bool)));

        QMetaObject::connectSlotsByName(SysTrayIconConfigWidget);
    } // setupUi

    void retranslateUi(QWidget *SysTrayIconConfigWidget)
    {
        SysTrayIconConfigWidget->setWindowTitle(QString());
        chkSysTrayOn->setText(QApplication::translate("SysTrayIconConfigWidget", "Dock into System Tray", 0, QApplication::UnicodeUTF8));
        chkRestoreOnHover->setText(QApplication::translate("SysTrayIconConfigWidget", "Restore Windows upon mouse hover", 0, QApplication::UnicodeUTF8));
        chkReplaceQuitButton->setText(QApplication::translate("SysTrayIconConfigWidget", "Replace main window's quit button by an iconify and quit button bar", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(SysTrayIconConfigWidget);
    } // retranslateUi

};

namespace Ui {
    class SysTrayIconConfigWidget: public Ui_SysTrayIconConfigWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYSTRAYICONCONFIGWIDGET_H
