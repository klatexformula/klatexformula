/********************************************************************************
** Form generated from reading ui file 'skinconfigwidget.ui'
**
** Created
**      by: Qt User Interface Compiler version 4.5.3
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_SKINCONFIGWIDGET_H
#define UI_SKINCONFIGWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SkinConfigWidget
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *lytSkinSelect;
    QLabel *label;
    QComboBox *cbxSkin;
    QPushButton *btnSaveCustom;
    QPushButton *btnDeleteSkin;
    QTextEdit *txtStyleSheet;

    void setupUi(QWidget *SkinConfigWidget)
    {
        if (SkinConfigWidget->objectName().isEmpty())
            SkinConfigWidget->setObjectName(QString::fromUtf8("SkinConfigWidget"));
        SkinConfigWidget->resize(400, 300);
        gridLayout = new QGridLayout(SkinConfigWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lytSkinSelect = new QHBoxLayout();
        lytSkinSelect->setObjectName(QString::fromUtf8("lytSkinSelect"));
        label = new QLabel(SkinConfigWidget);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        lytSkinSelect->addWidget(label);

        cbxSkin = new QComboBox(SkinConfigWidget);
        cbxSkin->setObjectName(QString::fromUtf8("cbxSkin"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(3);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(cbxSkin->sizePolicy().hasHeightForWidth());
        cbxSkin->setSizePolicy(sizePolicy1);

        lytSkinSelect->addWidget(cbxSkin);

        btnSaveCustom = new QPushButton(SkinConfigWidget);
        btnSaveCustom->setObjectName(QString::fromUtf8("btnSaveCustom"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(btnSaveCustom->sizePolicy().hasHeightForWidth());
        btnSaveCustom->setSizePolicy(sizePolicy2);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/pics/save.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnSaveCustom->setIcon(icon);

        lytSkinSelect->addWidget(btnSaveCustom);

        btnDeleteSkin = new QPushButton(SkinConfigWidget);
        btnDeleteSkin->setObjectName(QString::fromUtf8("btnDeleteSkin"));
        QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(btnDeleteSkin->sizePolicy().hasHeightForWidth());
        btnDeleteSkin->setSizePolicy(sizePolicy3);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/pics/delete.png"), QSize(), QIcon::Normal, QIcon::Off);
        btnDeleteSkin->setIcon(icon1);

        lytSkinSelect->addWidget(btnDeleteSkin);


        gridLayout->addLayout(lytSkinSelect, 0, 0, 1, 2);

        txtStyleSheet = new QTextEdit(SkinConfigWidget);
        txtStyleSheet->setObjectName(QString::fromUtf8("txtStyleSheet"));

        gridLayout->addWidget(txtStyleSheet, 1, 0, 1, 2);


        retranslateUi(SkinConfigWidget);

        QMetaObject::connectSlotsByName(SkinConfigWidget);
    } // setupUi

    void retranslateUi(QWidget *SkinConfigWidget)
    {
        SkinConfigWidget->setWindowTitle(QApplication::translate("SkinConfigWidget", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("SkinConfigWidget", "Skin:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnSaveCustom->setToolTip(QApplication::translate("SkinConfigWidget", "Save As New Custom Skin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnSaveCustom->setText(QString());
#ifndef QT_NO_TOOLTIP
        btnDeleteSkin->setToolTip(QApplication::translate("SkinConfigWidget", "Delete This Custom Skin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnDeleteSkin->setText(QString());
#ifndef QT_NO_TOOLTIP
        txtStyleSheet->setToolTip(QApplication::translate("SkinConfigWidget", "Edit the skin's Qt style sheet", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        Q_UNUSED(SkinConfigWidget);
    } // retranslateUi

};

namespace Ui {
    class SkinConfigWidget: public Ui_SkinConfigWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SKINCONFIGWIDGET_H
