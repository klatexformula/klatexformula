/***************************************************************************
 *   file klfguiutil.cpp
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2010 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id$ */

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QPushButton>
#include <QDebug>

#include "klfutil.h"
#include "klfguiutil.h"


// ----------------------------------------------


KLFProgressReporter::KLFProgressReporter(int min, int max, QObject *parent)
  : QObject(parent)
{
  pMin = min;
  pMax = max;
  pFinished = false;
}
KLFProgressReporter::~KLFProgressReporter()
{
  if (!pFinished) {  // make sure finished() is emitted.
    emit progress(pMax); // some connected clients just wait for maximum value progress
    emit finished();
  }
}

void KLFProgressReporter::doReportProgress(int value)
{
  if (pFinished) {
    qWarning()<<KLF_FUNC_NAME<<": Operation is already finished!";
    return;
  }
  emit progress(value);
  if (value == pMax) {
    emit finished();
    pFinished = true;
  }
}



// ---------------------------------------------------------



KLFProgressDialog::KLFProgressDialog(QString labelText, QWidget *parent)
  : QProgressDialog(parent)
{
  setup(false);
  init(labelText);
}
KLFProgressDialog::KLFProgressDialog(bool canCancel, QString labelText, QWidget *parent)
  : QProgressDialog(parent)
{
  setup(canCancel);
  init(labelText);
}
KLFProgressDialog::~KLFProgressDialog()
{
}

void KLFProgressDialog::setup(bool canCancel)
{
  setAutoClose(true);
  setAutoReset(true);
  setModal(true);
  //  setWindowModality(Qt::ApplicationModal);
  setWindowIcon(QIcon(":/pics/klatexformula-16.png"));
  setWindowTitle(tr("Progress"));
  QPushButton *cbtn = new QPushButton(tr("Cancel"), this);
  setCancelButton(cbtn);
  cbtn->setEnabled(canCancel);
}
void KLFProgressDialog::init(const QString& labelText)
{
  setDescriptiveText(labelText);
}

void KLFProgressDialog::setDescriptiveText(const QString& labelText)
{
  setLabelText(labelText);
  setFixedSize((int)(sizeHint().width()*1.3), (int)(sizeHint().height()*1.1));
}
void KLFProgressDialog::startReportingProgress(KLFProgressReporter *progressReporter,
					       const QString& descriptiveText)
{
  reset();
  setDescriptiveText(descriptiveText);
  setRange(progressReporter->min(), progressReporter->max());
  setValue(0);

  connect(progressReporter, SIGNAL(progress(int)), this, SLOT(setValue(int)));
}

void KLFProgressDialog::startReportingProgress(KLFProgressReporter *progressReporter)
{
  reset();
  setRange(progressReporter->min(), progressReporter->max());
  setValue(0);
  connect(progressReporter, SIGNAL(progress(int)), this, SLOT(setValue(int)));
}

void KLFProgressDialog::setValue(int value)
{
  //  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  klfDbg("value="<<value);
  QProgressDialog::setValue(value);
}

void KLFProgressDialog::paintEvent(QPaintEvent *event)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME);
  QProgressDialog::paintEvent(event);
}


// --------------------------



KLFPleaseWaitPopup::KLFPleaseWaitPopup(const QString& text, QWidget *parent)
  : QLabel(text, NULL,
	   Qt::SplashScreen|Qt::FramelessWindowHint|
	   Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint),
    pParentWidget(parent), pDisableUi(false), pGotPaintEvent(false)
{
  QFont f = font();
  f.setPointSize(QFontInfo(f).pointSize() + 2);
  setFont(f);
  setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  setWindowModality(Qt::ApplicationModal);
  // let this window be styled by skins
  setAttribute(Qt::WA_StyledBackground, true);
  setProperty("klfTopLevelWidget", QVariant(true));
  setStyleSheet(parent->window()->styleSheet());
  //  // set basic minimalistic style sheet to ensure that it is readable...
  //  setStyleSheet("background-color: #e0dfd8; background-image: url(); color: black;");

  int w = qMax( (int)(sizeHint().width() *1.3) , 500 );
  int h = qMax( (int)(sizeHint().height()*1.1) , 100 );
  setFixedSize(w, h);
}
KLFPleaseWaitPopup::~KLFPleaseWaitPopup()
{
  if (pDisableUi && pParentWidget != NULL)
    pParentWidget->setEnabled(true);
}

void KLFPleaseWaitPopup::setDisableUi(bool disableUi)
{
  pDisableUi = disableUi;
}

void KLFPleaseWaitPopup::showPleaseWait()
{
  QSize desktopSize;
  QDesktopWidget *dw = QApplication::desktop();
  if (dw != NULL) {
    desktopSize = dw->screenGeometry(this).size();
  } else {
    desktopSize = QSize(1024, 768); // assume some default, worst case widget is more left and higher
  }
  move(desktopSize.width()/2 - width()/2, desktopSize.height()/2 - height()/2);
  show();
  setStyleSheet(styleSheet());

  if (pDisableUi && pParentWidget != NULL)
    pParentWidget->setEnabled(false);

  while (!pGotPaintEvent)
    qApp->processEvents();
}

void KLFPleaseWaitPopup::mousePressEvent(QMouseEvent */*event*/)
{
  hide();
}

void KLFPleaseWaitPopup::paintEvent(QPaintEvent *event)
{
  pGotPaintEvent = true;
  QLabel::paintEvent(event);
}



// --------------------------


KLFDelayedPleaseWaitPopup::KLFDelayedPleaseWaitPopup(const QString& text, QWidget *callingWidget)
  : KLFPleaseWaitPopup(text, callingWidget), pDelay(1000)
{
  timer.start();
}
KLFDelayedPleaseWaitPopup::~KLFDelayedPleaseWaitPopup()
{
}
void KLFDelayedPleaseWaitPopup::setDelay(int ms)
{
  pDelay = ms;
}
void KLFDelayedPleaseWaitPopup::process()
{
  if (!pleaseWaitShown() && timer.elapsed() > pDelay)
    showPleaseWait();
  qApp->processEvents();
}



// ------------------------------------------------


KLFEnumComboBox::KLFEnumComboBox(QWidget *parent)
  : QComboBox(parent)
{
  setEnumValues(QList<int>(), QStringList());
  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(internalCurrentIndexChanged(int)));
}

KLFEnumComboBox::KLFEnumComboBox(const QList<int>& enumValues, const QStringList& enumTitles,
				 QWidget *parent)
  : QComboBox(parent)
{
  setEnumValues(enumValues, enumTitles);
  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(internalCurrentIndexChanged(int)));
}

KLFEnumComboBox::~KLFEnumComboBox()
{
}

void KLFEnumComboBox::setEnumValues(const QList<int>& enumValues, const QStringList& enumTitles)
{
  KLF_DEBUG_BLOCK(KLF_FUNC_NAME) ;
  klfDbg("enumValues="<<enumValues<<"; enumTitles="<<enumTitles);
  blockSignals(true);
  int savedCurrentIndex = currentIndex();
  if (enumValues.size() != enumTitles.size()) {
    qWarning()<<KLF_FUNC_NAME<<": enum value list and enum title list do not match!";
    return;
  }
  pEnumValueList = enumValues;
  clear();
  int k;
  for (k = 0; k < enumValues.size(); ++k) {
    pEnumValues[enumValues[k]] = enumTitles[k];
    insertItem(k, enumTitles[k], QVariant(enumValues[k]));
    pEnumCbxIndexes[enumValues[k]] = k;
  }
  if (savedCurrentIndex >= 0 && savedCurrentIndex < count())
    setCurrentIndex(savedCurrentIndex);
  blockSignals(false);
}

int KLFEnumComboBox::selectedValue() const
{
  return itemData(currentIndex()).toInt();
}

QString KLFEnumComboBox::enumText(int enumValue) const
{
  if (!pEnumValueList.contains(enumValue)) {
    qWarning()<<KLF_FUNC_NAME<<": "<<enumValue<<" is not a registered valid enum value!";
    return QString();
  }
  return pEnumValues[enumValue];
}

void KLFEnumComboBox::setSelectedValue(int val)
{
  if (!pEnumCbxIndexes.contains(val)) {
    qWarning()<<KLF_FUNC_NAME<<": "<<val<<" is not a registered valid enum value!";
    return;
  }
  setCurrentIndex(pEnumCbxIndexes[val]);
}

void KLFEnumComboBox::internalCurrentIndexChanged(int index)
{
  emit selectedValueChanged(itemData(index).toInt());
}

