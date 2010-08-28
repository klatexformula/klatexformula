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

#include <math.h>

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
  pProgressReporter = NULL;
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

  // disconnect any previous progress reporter object
  if (pProgressReporter != NULL)
    disconnect(pProgressReporter, 0, this, SLOT(setValue(int)));
  // and connect to this new one
  connect(progressReporter, SIGNAL(progress(int)), this, SLOT(setValue(int)));
}

void KLFProgressDialog::startReportingProgress(KLFProgressReporter *progressReporter)
{
  reset();
  setRange(progressReporter->min(), progressReporter->max());
  setValue(0);
  // disconnect any previous progress reporter object
  disconnect(0, 0, this, SLOT(setValue(int)));
  // and connect to this new one
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


// ------------------------


KLFUnitChooser::KLFUnitChooser(QWidget *parent)
  : QComboBox(parent)
{
  connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(internalCurrentIndexChanged(int)));
}
KLFUnitChooser::~KLFUnitChooser()
{
}

void KLFUnitChooser::setUnits(const QStringList& unitstrlist)
{
  QList<Unit> units;
  int k;
  for (k = 0; k < unitstrlist.size(); ++k) {
    QStringList parts = unitstrlist[k].split('=');
    if (parts.size() != 3) {
      qWarning()<<KLF_FUNC_NAME<<": Invalid unit specification: "<<unitstrlist[k];
      continue;
    }
    Unit u;
    u.name = parts[0];
    u.abbrev = parts[1];
    u.factor = parts[2].toDouble();
    units << u;
  }
  setUnitList(units);
}

void KLFUnitChooser::setUnitList(const QList<Unit>& unitlist)
{
  clear(); // clear combo box
  pUnits = unitlist;
  int k;
  for (k = 0; k < pUnits.size(); ++k) {
    Unit u = pUnits[k];
    // add this cbx item
    addItem(u.name, QVariant::fromValue<Unit>(u));
  }
}

QStringList KLFUnitChooser::unitStrList() const
{
  QStringList l;
  int k;
  for (k = 0; k < pUnits.size(); ++k)
    l << QString("%2=%3=%1").arg(pUnits[k].factor, 0, 'g').arg(pUnits[k].name, pUnits[k].abbrev);
  return l;
}

void KLFUnitChooser::setCurrentUnit(const QString& unitname)
{
  int k;
  for (k = 0; k < count(); ++k) {
    if (itemData(k).value<Unit>().name == unitname) {
      setCurrentIndex(k);
      return;
    }
  }
  qWarning()<<KLF_FUNC_NAME<<": unit "<<unitname<<" not found.";
}

void KLFUnitChooser::internalCurrentIndexChanged(int index)
{
  Unit u = itemData(index).value<Unit>();
  emit unitChanged(u.name);
  emit unitChanged(u.factor);
}


// -------------------------


KLFUnitSpinBox::KLFUnitSpinBox(QWidget *parent)
  : QDoubleSpinBox(parent)
{
  pUnitFactor = 1.0f;
  connect(this, SIGNAL(valueChanged(double)), this, SLOT(internalValueChanged(double)));
}
KLFUnitSpinBox::~KLFUnitSpinBox()
{
}

void KLFUnitSpinBox::setUnit(double unitfactor)
{
  double curValue = value();
  double curMinimum = minimum();
  double curMaximum = maximum();
  double curUnitFactor = pUnitFactor;
  int curPrecision = decimals();

  klfDbg("unitfactor="<<unitfactor<<" cur: val="<<curValue<<",min="<<curMinimum<<",max="<<curMaximum
	 <<",prec="<<curPrecision) ;

  pUnitFactor = unitfactor;

  // calculate the right number of decimal places.
  // due to round-off errors, we need to first determine how many decimals we started
  // off with, then re-calculate the number of decimals.
  int unitRefDecimals = curPrecision - (int)( log((double)curUnitFactor)/log((double)10.0)  + 0.5 );

  setDecimals(unitRefDecimals + (int)( log((double)pUnitFactor)/log((double)10.0) + 0.5 ) );

  // set the appropriate range
  setMinimum(curMinimum * curUnitFactor / pUnitFactor);
  setMaximum(curMaximum * curUnitFactor / pUnitFactor);

  // and set the value
  setValue(curValue * curUnitFactor / pUnitFactor);
}

void KLFUnitSpinBox::setValueInRefUnit(double value)
{
  setValue(value / pUnitFactor);
}


void KLFUnitSpinBox::internalValueChanged(double valueInExtUnits)
{
  klfDbg("val in ext. units="<<valueInExtUnits<<"; our unitfactor="<<pUnitFactor) ;
  emit valueInRefUnitChanged(valueInExtUnits / pUnitFactor);
}







