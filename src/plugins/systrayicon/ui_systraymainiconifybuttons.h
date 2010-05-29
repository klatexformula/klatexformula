/********************************************************************************
** Form generated from reading ui file 'systraymainiconifybuttons.ui'
**
** Created
**      by: Qt User Interface Compiler version 4.5.3
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_SYSTRAYMAINICONIFYBUTTONS_H
#define UI_SYSTRAYMAINICONIFYBUTTONS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SysTrayMainIconifyButtons
{
public:
    QHBoxLayout *lyt_SysTrayMainIconifyButtons;
    QPushButton *btnIconify;
    QPushButton *btnQuit;

    void setupUi(QWidget *SysTrayMainIconifyButtons)
    {
        if (SysTrayMainIconifyButtons->objectName().isEmpty())
            SysTrayMainIconifyButtons->setObjectName(QString::fromUtf8("SysTrayMainIconifyButtons"));
        SysTrayMainIconifyButtons->resize(379, 29);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(SysTrayMainIconifyButtons->sizePolicy().hasHeightForWidth());
        SysTrayMainIconifyButtons->setSizePolicy(sizePolicy);
        lyt_SysTrayMainIconifyButtons = new QHBoxLayout(SysTrayMainIconifyButtons);
        lyt_SysTrayMainIconifyButtons->setMargin(0);
        lyt_SysTrayMainIconifyButtons->setObjectName(QString::fromUtf8("lyt_SysTrayMainIconifyButtons"));
        lyt_SysTrayMainIconifyButtons->setSizeConstraint(QLayout::SetMinimumSize);
        btnIconify = new QPushButton(SysTrayMainIconifyButtons);
        btnIconify->setObjectName(QString::fromUtf8("btnIconify"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(btnIconify->sizePolicy().hasHeightForWidth());
        btnIconify->setSizePolicy(sizePolicy1);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/pics/iconify.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnIconify->setIcon(icon);

        lyt_SysTrayMainIconifyButtons->addWidget(btnIconify);

        btnQuit = new QPushButton(SysTrayMainIconifyButtons);
        btnQuit->setObjectName(QString::fromUtf8("btnQuit"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(btnQuit->sizePolicy().hasHeightForWidth());
        btnQuit->setSizePolicy(sizePolicy2);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/pics/closehide.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnQuit->setIcon(icon1);

        lyt_SysTrayMainIconifyButtons->addWidget(btnQuit);


        retranslateUi(SysTrayMainIconifyButtons);

        QMetaObject::connectSlotsByName(SysTrayMainIconifyButtons);
    } // setupUi

    void retranslateUi(QWidget *SysTrayMainIconifyButtons)
    {
        SysTrayMainIconifyButtons->setWindowTitle(QString());
#ifndef QT_NO_TOOLTIP
        btnIconify->setToolTip(QApplication::translate("SysTrayMainIconifyButtons", "Minimize Window to System Tray", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        btnIconify->setWhatsThis(QApplication::translate("SysTrayMainIconifyButtons", "This button minimizes KLatexFormula's main window to the system tray, without quitting.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        btnIconify->setText(QApplication::translate("SysTrayMainIconifyButtons", "Iconify", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnQuit->setToolTip(QApplication::translate("SysTrayMainIconifyButtons", "Quit KLatexFormula [Ctrl+Q]", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        btnQuit->setWhatsThis(QApplication::translate("SysTrayMainIconifyButtons", "This button quits KLatexFormula, instead of minimizing the window to the system tray.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        btnQuit->setText(QString());
        btnQuit->setShortcut(QApplication::translate("SysTrayMainIconifyButtons", "Ctrl+Q", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(SysTrayMainIconifyButtons);
    } // retranslateUi

};

namespace Ui {
    class SysTrayMainIconifyButtons: public Ui_SysTrayMainIconifyButtons {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYSTRAYMAINICONIFYBUTTONS_H
